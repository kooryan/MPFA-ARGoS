#include "CPFA_controller.h"
#include <unistd.h>

CPFA_controller::CPFA_controller() :
	RNG(argos::CRandom::CreateRNG("argos")),
	isInformed(false),
	isHoldingFood(false),
	isUsingSiteFidelity(false),
	isGivingUpSearch(false),
	ResourceDensity(0),
	MaxTrailSize(50),
	SearchTime(0),
	MPFA_state(DEPARTING),
	LoopFunctions(NULL),
 ClosestNest(NULL), //qilu 09/07
	survey_count(0),
	isUsingPheromone(0),
 SiteFidelityPosition(1000, 1000), //qilu 09/07/2016
 searchingTime(0),
 travelingTime(0),
 startTime(0),
 updateFidelity(false)
{
}

void CPFA_controller::Init(argos::TConfigurationNode &node) {
	compassSensor   = GetSensor<argos::CCI_PositioningSensor>("positioning");
	wheelActuator   = GetActuator<argos::CCI_DifferentialSteeringActuator>("differential_steering");
	proximitySensor = GetSensor<argos::CCI_FootBotProximitySensor>("footbot_proximity");
	argos::TConfigurationNode settings = argos::GetNode(node, "settings");

	argos::GetNodeAttribute(settings, "FoodDistanceTolerance",   FoodDistanceTolerance);
	argos::GetNodeAttribute(settings, "TargetDistanceTolerance", TargetDistanceTolerance);
	argos::GetNodeAttribute(settings, "NestDistanceTolerance", NestDistanceTolerance);
	argos::GetNodeAttribute(settings, "NestAngleTolerance",    NestAngleTolerance);
	argos::GetNodeAttribute(settings, "TargetAngleTolerance",    TargetAngleTolerance);
	argos::GetNodeAttribute(settings, "SearchStepSize",          SearchStepSize);
	argos::GetNodeAttribute(settings, "RobotForwardSpeed",       RobotForwardSpeed);
	argos::GetNodeAttribute(settings, "RobotRotationSpeed",      RobotRotationSpeed);
	argos::GetNodeAttribute(settings, "ResultsDirectoryPath",      results_path);
	argos::GetNodeAttribute(settings, "DestinationNoiseStdev",      DestinationNoiseStdev);
	argos::GetNodeAttribute(settings, "PositionNoiseStdev",      PositionNoiseStdev);

	argos::CVector2 p(GetPosition());
	SetStartPosition(argos::CVector3(p.GetX(), p.GetY(), 0.0));


FoodDistanceTolerance *= FoodDistanceTolerance;
	SetIsHeadingToNest(true);
	//SetTarget(argos::CVector2(0,0));
	//qilu 10/21/2016 Let robots start to search immediately
	SetTarget(p);
	
 controllerID= GetId();//qilu 07/26/2016
}

void CPFA_controller::ControlStep() {
	/*
	ofstream log_output_stream;
	log_output_stream.open("cpfa_log.txt", ios::app);

	// depart from nest after food drop off or simulation start
	if (isHoldingFood) log_output_stream << "(Carrying) ";
	
	switch(CPFA_state)  {
		case DEPARTING:
			if (isUsingSiteFidelity) {
				log_output_stream << "DEPARTING (Fidelity): "
					<< GetTarget().GetX() << ", " << GetTarget().GetY()
					<< endl;
			} else if (isInformed) {
				log_output_stream << "DEPARTING (Waypoint): "
				<< GetTarget().GetX() << ", " << GetTarget().GetY() << endl;
			} else {
				log_output_stream << "DEPARTING (Searching): "
				<< GetTarget().GetX() << ", " << GetTarget().GetY() << endl;
			}
			break;
		// after departing(), once conditions are met, begin searching()
		case SEARCHING:
			if (isInformed) log_output_stream << "SEARCHING: Informed" << endl;     
			else log_output_stream << "SEARCHING: UnInformed" << endl;
			break;
		// return to nest after food pick up or giving up searching()
		case RETURNING:
			log_output_stream << "RETURNING" << endl;
			break;
		case SURVEYING:
			log_output_stream << "SURVEYING" << endl;
			break;
		default:
			log_output_stream << "Unknown state" << endl;
	}
	*/

	// Add line so we can draw the trail

	CVector3 position3d(GetPosition().GetX(), GetPosition().GetY(), 0.00);
	CVector3 target3d(previous_position.GetX(), previous_position.GetY(), 0.00);
	CRay3 targetRay(target3d, position3d);
	myTrail.push_back(targetRay);
	LoopFunctions->TargetRayList.push_back(targetRay);
	LoopFunctions->TargetRayColorList.push_back(TrailColor);

	previous_position = GetPosition();

	//UpdateTargetRayList();
	CPFA();
	Move();
}

void CPFA_controller::Reset() {
 isHoldingFood   = false;
    isInformed      = false;
    SearchTime      = 0;
    ResourceDensity = 0;
    collisionDelay = 0;
    
  	 LoopFunctions->CollisionTime=0; //qilu 09/26/2016
	   //LoopFunctions->currCollisionTime =0; //qilu 09/26/2016
    //LoopFunctions->lastCollisionTime =0; //qilu 09/26/2016
    
    
    /* Set LED color */
    /* m_pcLEDs->SetAllColors(CColor::BLACK); //qilu 09/04 */
    SetTarget(ClosestNest->GetLocation()); //qilu 09/08
    SiteFidelityPosition = ClosestNest->GetLocation();//qilu 09/08
    
    
    
    
    TrailToShare.clear();
    TrailToFollow.clear();
}

bool CPFA_controller::IsHoldingFood() {
		return isHoldingFood;
}

bool CPFA_controller::IsUsingSiteFidelity() {
		return isUsingSiteFidelity;
}

void CPFA_controller::CPFA() {
	
	switch(MPFA_state) {
		// depart from nest after food drop off or simulation start
		case DEPARTING:
			//argos::LOG << "DEPARTING" << std::endl;
			SetIsHeadingToNest(false);
			Departing();
			break;
		// after departing(), once conditions are met, begin searching()
		case SEARCHING:
			//argos::LOG << "SEARCHING" << std::endl;
			SetIsHeadingToNest(false);
			if((SimulationTick() % (SimulationTicksPerSecond() / 2)) == 0) {
				Searching();
			}
			break;
		// return to nest after food pick up or giving up searching()
		case RETURNING:
			//argos::LOG << "RETURNING" << std::endl;
			SetIsHeadingToNest(true);
			Returning();
			break;
		case SURVEYING:
			//argos::LOG << "SURVEYING" << std::endl;
			SetIsHeadingToNest(false);
			Surveying();
			break;
	}
}

bool CPFA_controller::IsInTheNest() {
	//return ((GetPosition() - LoopFunctions->NestPosition).SquareLength()
		//< LoopFunctions->NestRadiusSquared);
  for (size_t i=0; i<LoopFunctions->Nests.size(); i++) { //qilu 07/26/2016
        if ((GetPosition() - LoopFunctions->Nests[i].GetLocation()).SquareLength()<4.0*LoopFunctions->NestRadiusSquared) {
            return true;
        }
  }
  return false;
}

void CPFA_controller::SetLoopFunctions(CPFA_loop_functions* lf) {
	LoopFunctions = lf;

	// Initialize the SiteFidelityPosition
	//SiteFidelityPosition = LoopFunctions->NestPosition;
SiteFidelityPosition = CVector2(0,0); //qilu 07/26/2016

	// Create the output file here because it needs LoopFunctions
		
	// Name the results file with the current time and date
	time_t t = time(0);   // get time now
	struct tm * now = localtime(&t);
	stringstream ss;

	char hostname[1024];                                                   
	hostname[1023] = '\0';                    
	gethostname(hostname, 1023);  

	ss << "CPFA-"<<GIT_BRANCH<<"-"<<GIT_COMMIT_HASH<<"-"
		<< hostname << '-'
		<< getpid() << '-'
		<< (now->tm_year) << '-'
		<< (now->tm_mon + 1) << '-'
		<<  now->tm_mday << '-'
		<<  now->tm_hour << '-'
		<<  now->tm_min << '-'
		<<  now->tm_sec << ".csv";

		string results_file_name = ss.str();
		results_full_path = results_path+"/"+results_file_name;

	// Only the first robot should do this:	 
	if (GetId().compare("CPFA_0") == 0) {
		/*
		ofstream results_output_stream;
		results_output_stream.open(results_full_path, ios::app);
		results_output_stream << "NumberOfRobots, "
			<< "TargetDistanceTolerance, "
			<< "TargetAngleTolerance, "
			<< "FoodDistanceTolerance, "
			<< "RobotForwardSpeed, "
			<< "RobotRotationSpeed, "
			<< "RandomSeed, "
			<< "ProbabilityOfSwitchingToSearching, "
			<< "ProbabilityOfReturningToNest, "
			<< "UninformedSearchVariation, "   
			<< "RateOfInformedSearchDecay, "   
			<< "RateOfSiteFidelity, "          
			<< "RateOfLayingPheromone, "       
			<< "RateOfPheromoneDecay" << endl
			<< LoopFunctions->getNumberOfRobots() << ", "
			<< CSimulator::GetInstance().GetRandomSeed() << ", "  
			<< TargetDistanceTolerance << ", "
			<< TargetAngleTolerance << ", "
			<< FoodDistanceTolerance << ", "
			<< RobotForwardSpeed << ", "
			<< RobotRotationSpeed << ", "
			<< LoopFunctions->getProbabilityOfSwitchingToSearching() << ", "
			<< LoopFunctions->getProbabilityOfReturningToNest() << ", "
			<< LoopFunctions->getUninformedSearchVariation() << ", "
			<< LoopFunctions->getRateOfInformedSearchDecay() << ", "
			<< LoopFunctions->getRateOfSiteFidelity() << ", "
			<< LoopFunctions->getRateOfLayingPheromone() << ", "
			<< LoopFunctions->getRateOfPheromoneDecay()
			<< endl;
				
			results_output_stream.close();
		*/
	}

}

void CPFA_controller::Departing()
{
     //LOG<<"Departing..."<<endl;
	    argos::Real distanceToTarget = (GetPosition() - GetTarget()).Length();
	    argos::Real randomNumber = RNG->Uniform(argos::CRange<argos::Real>(0.0, 1.0));

	    /*
	    ofstream log_output_stream;
	    log_output_stream.open("cpfa_log.txt", ios::app);
	    log_output_stream << "Distance to target: " << distanceToTarget << endl;
	    log_output_stream << "Current Position: " << GetPosition() << ", Target: " << GetTarget() << endl;
	    log_output_stream.close();
	    */

    	/* When not informed, continue to travel until randomly switching to the searching state. */
	    if((SimulationTick() % (SimulationTicksPerSecond() / 2)) == 0) {
		       if(isInformed == false){
		           if(SimulationTick()%(5*SimulationTicksPerSecond())==0 && randomNumber < LoopFunctions->ProbabilityOfSwitchingToSearching){
                 //LOG<<"Switch to search..."<<endl;
                 Stop();
                 SearchTime = 0;
                 MPFA_state = SEARCHING;
                 travelingTime+=SimulationTick()-startTime;//qilu 10/22
                 startTime = SimulationTick();//qilu 10/22
            
                 argos::Real USV = LoopFunctions->UninformedSearchVariation.GetValue();
                 argos::Real rand = RNG->Gaussian(USV);
                 argos::CRadians rotation(rand);
                 argos::CRadians angle1(rotation.UnsignedNormalize());
                 argos::CRadians angle2(GetHeading().UnsignedNormalize());
                 argos::CRadians turn_angle(angle1 + angle2);
                 argos::CVector2 turn_vector(SearchStepSize, turn_angle);
                 SetIsHeadingToNest(false);
                 SetTarget(turn_vector + GetPosition());
             }
             else if(distanceToTarget < TargetDistanceTolerance){
                 SetRandomSearchLocation();
             }
         }
     }
	
     /* Are we informed? I.E. using site fidelity or pheromones. */	
     if(isInformed && distanceToTarget < TargetDistanceTolerance) {
          //ofstream log_output_stream;
          //log_output_stream.open("cpfa_log.txt", ios::app);
          //log_output_stream << "Reached waypoint: " << SiteFidelityPosition << endl;
        
          SearchTime = 0;
          MPFA_state = SEARCHING;
          travelingTime+=SimulationTick()-startTime;//qilu 10/22
          startTime = SimulationTick();//qilu 10/22
            
          if(isUsingSiteFidelity) {
               isUsingSiteFidelity = false;
               SetFidelityList();
               //log_output_stream << "After SetFidelityList: " << SiteFidelityPosition << endl;
               //log_output_stream.close();
          }
     }


}

void CPFA_controller::Searching() {
 //LOG<<"Searching..."<<endl;
	// "scan" for food only every half of a second
	if((SimulationTick() % (SimulationTicksPerSecond() / 2)) == 0) {
		SetHoldingFood();
	}
	// When not carrying food, calculate movement.
	if(IsHoldingFood() == false) {
		   argos::CVector2 distance = GetPosition() - GetTarget();
		   argos::Real     random   = RNG->Uniform(argos::CRange<argos::Real>(0.0, 1.0));
     
     // If we reached our target search location, set a new one. The 
     // new search location calculation is different based on whether
     // we are currently using informed or uninformed search.
     if(distance.SquareLength() < TargetDistanceTolerance) {
         // randomly give up searching
         if(SimulationTick()%(5*SimulationTicksPerSecond())==0 && random < LoopFunctions->ProbabilityOfReturningToNest) {
             
             SetClosestNest();//qilu 07/26/2016
             SetIsHeadingToNest(true);
             //SetTarget(LoopFunctions->NestPosition);
             SetTarget(ClosestNest->GetLocation());
             isGivingUpSearch = true;
             ClosestNest->FidelityList.erase(controllerID); //09/07/2016
             ClosestNest->DensityOnFidelity.erase(controllerID); //09/11/2016
             SiteFidelityPosition= CVector2(10000,10000); //09/07/2016
             isUsingSiteFidelity = false; //qilu 09/07/2016
             updateFidelity = false; //qilu 09/07/2016
             MPFA_state = RETURNING;
             searchingTime+=SimulationTick()-startTime;//qilu 10/22
             startTime = SimulationTick();//qilu 10/22
             /*
             ofstream log_output_stream;
             log_output_stream.open("giveup.txt", ios::app);
             log_output_stream << "Give up: " << SimulationTick() / SimulationTicksPerSecond() << endl;
             log_output_stream.close();
             */
             return; 
         }
         argos::Real USCV = LoopFunctions->UninformedSearchVariation.GetValue();
         argos::Real rand = RNG->Gaussian(USCV);

         // uninformed search
         if(isInformed == false) {
          argos::CRadians rotation(rand);
          argos::CRadians angle1(rotation);
          argos::CRadians angle2(GetHeading());
          argos::CRadians turn_angle(angle1 + angle2);
          argos::CVector2 turn_vector(SearchStepSize, turn_angle);
      
          //argos::LOG << "UNINFORMED SEARCH: rotation: " << angle1 << std::endl;
          //argos::LOG << "UNINFORMED SEARCH: old heading: " << angle2 << std::endl;
      
          /*
          ofstream log_output_stream;
          log_output_stream.open("uninformed_angle1.log", ios::app);
          log_output_stream << angle1.GetValue() << endl;
          log_output_stream.close();
      
          log_output_stream.open("uninformed_angle2.log", ios::app);
          log_output_stream << angle2.GetValue() << endl;
          log_output_stream.close();
      
          log_output_stream.open("uninformed_turning_angle.log", ios::app);
          log_output_stream << turn_angle.GetValue() << endl;
          log_output_stream.close();
          */
          SetIsHeadingToNest(false);
          SetTarget(turn_vector + GetPosition());
         }
         // informed search
         else{
          
              SetIsHeadingToNest(false);
              
              if(IsAtTarget()) {
                  size_t          t           = SearchTime++;
                  argos::Real     twoPi       = (argos::CRadians::TWO_PI).GetValue();
                  argos::Real     pi          = (argos::CRadians::PI).GetValue();
                  argos::Real     isd         = LoopFunctions->RateOfInformedSearchDecay;
                  /*argos::Real     correlation = GetExponentialDecay((2.0 * twoPi) - LoopFunctions->UninformedSearchVariation.GetValue(), t, isd);
                  argos::Real     rand = RNG->Gaussian(correlation + LoopFunctions->UninformedSearchVariation.GetValue());
                       */ //qilu 09/24/2016
                  Real correlation = GetExponentialDecay(rand, t, isd);
                  //argos::CRadians rotation(GetBound(rand, -pi, pi));
                  argos::CRadians rotation(GetBound(correlation, -pi, pi));//qilu 09/24/2016
                  argos::CRadians angle1(rotation);
                  argos::CRadians angle2(GetHeading());
                  argos::CRadians turn_angle(angle2 + angle1);
                  argos::CVector2 turn_vector(SearchStepSize, turn_angle);
          
                  //argos::LOG << "INFORMED SEARCH: rotation: " << angle1 << std::endl;
                  //argos::LOG << "INFORMED SEARCH: old heading: " << angle2 << std::endl;
          
                  /*
                  ofstream log_output_stream;
                  log_output_stream.open("informed_angle1.log", ios::app);
                  log_output_stream << angle1.GetValue() << endl;
                  log_output_stream.close();
          
                  log_output_stream.open("informed_angle2.log", ios::app);
                  log_output_stream << angle2.GetValue() << endl;
                  log_output_stream.close();
          
                  log_output_stream.open("informed_turning_angle.log", ios::app);
                  log_output_stream << turn_angle.GetValue() << endl;
                  log_output_stream.close();
                  */
                  SetTarget(turn_vector + GetPosition());
              }
         }
		   }
		   else {
			      //argos::LOG << "SEARCH: Haven't reached destination. " << GetPosition() << "," << GetTarget() << std::endl;
		   }
	}
	else {
		   //argos::LOG << "SEARCH: Carrying food." << std::endl;
	}

	// Food has been found, change state to RETURNING and go to the nest
	//else {
	//	SetTarget(LoopFunctions->NestPosition);
	//	CPFA_state = RETURNING;
	//}
}

// Cause the robot to rotate in place as if surveying the surrounding targets
// Turns 36 times by 10 degrees
void CPFA_controller::Surveying() {
 //LOG<<"Surveying..."<<endl;
	if (survey_count <= 4) { 
		CRadians rotation(survey_count*3.14/2); // divide by 10 so the vecot is small and the linear motion is minimized
		argos::CVector2 turn_vector(SearchStepSize, rotation.SignedNormalize());
			
		SetIsHeadingToNest(true); // Turn off error for this
		SetTarget(turn_vector + GetPosition());
		/*
		ofstream log_output_stream;
		log_output_stream.open("log.txt", ios::app);
		log_output_stream << (GetHeading() - rotation ).SignedNormalize() << ", "  << SearchStepSize << ", "<< rotation << ", " <<  turn_vector << ", " << GetHeading() << ", " << survey_count << endl;
		log_output_stream.close();
		*/
		
		if(fabs((GetHeading() - rotation).SignedNormalize().GetValue()) < TargetAngleTolerance.GetValue()) survey_count++;
			//else Keep trying to reach the turning angle
	}
	// Set the survey countdown
	else {
		SetIsHeadingToNest(true); // Turn off error for this
		SetTarget(ClosestNest->GetLocation()); //qilu 07/26/2016
		MPFA_state = RETURNING;
		searchingTime+=SimulationTick()-startTime;//qilu 10/22
        startTime = SimulationTick();//qilu 10/22
        
		survey_count = 0; // Reset
	}
}


/*****
 * RETURNING: Stay in this state until the robot has returned to the nest.
 * This state is triggered when a robot has found food or when it has given
 * up on searching and is returning to the nest.
 *****/
void CPFA_controller::Returning() {
 //LOG<<"Returning..."<<endl;
	//SetHoldingFood();
	//SetTarget(LoopFunctions->NestPosition);

	// Are we there yet? (To the nest, that is.)
	if(IsInTheNest()) {
		    // Based on a Poisson CDF, the robot may or may not create a pheromone
		    // located at the last place it picked up food.
		    argos::Real poissonCDF_pLayRate    = GetPoissonCDF(ResourceDensity, LoopFunctions->RateOfLayingPheromone);
		    argos::Real poissonCDF_sFollowRate = GetPoissonCDF(ResourceDensity, LoopFunctions->RateOfSiteFidelity);
		    argos::Real r1 = RNG->Uniform(argos::CRange<argos::Real>(0.0, 1.0));
		    argos::Real r2 = RNG->Uniform(argos::CRange<argos::Real>(0.0, 1.0));
               ClosestNest->visited_time_point_in_minute = (argos::Real)(SimulationTick() / SimulationTicksPerSecond())/60;
               if (isHoldingFood) { 
			       
          //drop off the food qilu 09/07/2016
         /* argos::CVector2 placementPosition;
          placementPosition.Set(ClosestNest->GetLocation().GetX()+RNG->Gaussian(0.1, 0), ClosestNest->GetLocation().GetY()+RNG->Gaussian(0.1, 0));
          
          while((placementPosition-ClosestNest->GetLocation()).SquareLength()>pow(LoopFunctions->NestRadius/2.0-LoopFunctions->FoodRadius, 2))
              placementPosition.Set(ClosestNest->GetLocation().GetX()+RNG->Gaussian(0.1, 0), ClosestNest->GetLocation().GetY()+RNG->Gaussian(0.1, 0));
     
          ClosestNest->FoodList.push_back(placementPosition);*/
          //Update the location of the nest qilu 09/10
          num_targets_collected++;
          ClosestNest->num_collected_tags++;
	      LoopFunctions->currNumCollectedFood++; 
          ClosestNest->UpdateNestLocation();
         
          //Update the collected resources in the nest after updating the location of the nest
          /*for (size_t i=0; i<ClosestNest->FoodList.size(); i++) {
             if((ClosestNest->FoodList[i] - ClosestNest->GetLocation()).SquareLength() > pow(LoopFunctions->NestRadius-LoopFunctions->FoodRadius, 2)){
                 LoopFunctions->FoodList.push_back(ClosestNest->FoodList[i]);
                 LoopFunctions->FoodColoringList.push_back(argos::CColor::BLACK);
                 ClosestNest->FoodList.erase(ClosestNest->FoodList.begin()+i);
             }
          }*/
       
          //Update the food list and check whether there is some unknown resources have already been in the nest
          /*std::vector<argos::CVector2> newFoodList;
          std::vector<argos::CColor> newFoodColoringList;
    
          for(size_t i = 0; i < LoopFunctions->FoodList.size(); i++) {
              if((ClosestNest->GetLocation() - LoopFunctions->FoodList[i]).SquareLength() < pow(LoopFunctions->NestRadius-LoopFunctions->FoodRadius, 2)) {
                  num_targets_collected++;
              }
              else{
                  newFoodList.push_back(LoopFunctions->FoodList[i]);
                  newFoodColoringList.push_back(LoopFunctions->FoodColoringList[i]);
              }
          }
          LoopFunctions->FoodList = newFoodList;
          LoopFunctions->FoodColoringList = newFoodColoringList; //qilu 09/12/2016
          */
          
          // Record that a target has been retrieved
          /*num_targets_collected =0;
          for(size_t n=0; n<LoopFunctions->Nests.size(); n++){
           //LOG<<"FoodList "<<n<<" size ="<<LoopFunctions->Nests[n].FoodList.size()<<endl;
              num_targets_collected += LoopFunctions->Nests[n].FoodList.size();
          }*/
      
          LoopFunctions->setScore(num_targets_collected);

          if(poissonCDF_pLayRate > r1 && updateFidelity) {
		TrailToShare.push_back(ClosestNest->GetLocation()); //qilu 07/26/2016
                argos::Real timeInSeconds = (argos::Real)(SimulationTick() / SimulationTicksPerSecond());
	        Pheromone sharedPheromone(SiteFidelityPosition, TrailToShare, timeInSeconds, LoopFunctions->RateOfPheromoneDecay, ResourceDensity);
                ClosestNest->PheromoneList.push_back(sharedPheromone);//qilu 09/08/2016
                ClosestNest->DensityOnFidelity.erase(controllerID); //09/11/2016 if it creates a pheromone trail, the sensed density on site fidelity should be removed. Otherwise, there is a repeated information.
                
                sharedPheromone.Deactivate(); // make sure this won't get re-added later...
          }
           TrailToShare.clear();
            }

		    // Determine probabilistically whether to use site fidelity, pheromone
		    // trails, or random search.
		    //ofstream log_output_stream;
		    //log_output_stream.open("cpfa_log.txt", ios::app);
		    //log_output_stream << "At the nest." << endl;	    
		 
		    // use site fidelity
		    if(updateFidelity && poissonCDF_sFollowRate > r2) {
			    //log_output_stream << "Using site fidelity" << endl;
			        SetIsHeadingToNest(false);
			        SetTarget(SiteFidelityPosition);
			        isInformed = true;
		    }
      // use pheromone waypoints
      else if(SetTargetPheromone()) {
          //log_output_stream << "Using site pheremone" << endl;
          isInformed = true;
          isUsingSiteFidelity = false;
      }
       // use random search
      else {
           //log_output_stream << "Using random search" << endl;
            SetRandomSearchLocation();
            isInformed = false;
            isUsingSiteFidelity = false;
      }

		    

		    isGivingUpSearch = false;
		    MPFA_state = DEPARTING; 
		    travelingTime+=SimulationTick()-startTime;//qilu 10/22
            startTime = SimulationTick();//qilu 10/22  
		    isHoldingFood = false; 
      
          
      //ClosestNest.FoodList = LoopFunctions->UpdateCollectedFoodList(ClosestNest.FoodList);
		    //log_output_stream.close();
    }
	// Take a small step towards the nest so we don't overshoot by too much is we miss it
	else {
		   SetIsHeadingToNest(true); // Turn off error for this
		   //SetTarget(LoopFunctions->NestPosition);
           SetTarget(ClosestNest->GetLocation()); //qilu 07/26/2016
	}		
}

void CPFA_controller::SetRandomSearchLocation() {
	argos::Real random_wall = RNG->Uniform(argos::CRange<argos::Real>(0.0, 1.0));
	argos::Real x = 0.0, y = 0.0;

	/* north wall */
	if(random_wall < 0.25) {
		x = RNG->Uniform(ForageRangeX);
		y = ForageRangeY.GetMax();
	}
	/* south wall */
	else if(random_wall < 0.5) {
		x = RNG->Uniform(ForageRangeX);
		y = ForageRangeY.GetMin();
	}
	/* east wall */
	else if(random_wall < 0.75) {
		x = ForageRangeX.GetMax();
		y = RNG->Uniform(ForageRangeY);
	}
	/* west wall */
	else {
		x = ForageRangeX.GetMin();
		y = RNG->Uniform(ForageRangeY);
	}
		
	SetIsHeadingToNest(true); // Turn off error for this
	SetTarget(argos::CVector2(x, y));
}

/*****
 * Check if the iAnt is finding food. This is defined as the iAnt being within
 * the distance tolerance of the position of a food item. If the iAnt has found
 * food then the appropriate boolean flags are triggered.
 *****/
void CPFA_controller::SetHoldingFood() {
	// Is the iAnt already holding food?
	if(IsHoldingFood() == false) {
		// No, the iAnt isn't holding food. Check if we have found food at our
		// current position and update the food list if we have.

		    std::vector<argos::CVector2> newFoodList;
		    std::vector<argos::CColor> newFoodColoringList;
		    size_t i = 0, j = 0;
      if(MPFA_state != RETURNING){
		         for(i = 0; i < LoopFunctions->FoodList.size(); i++) {
			            if((GetPosition() - LoopFunctions->FoodList[i]).SquareLength() < FoodDistanceTolerance ) {
				               // We found food! Calculate the nearby food density.
				               isHoldingFood = true;
				               MPFA_state = SURVEYING;
				               //searchingTime+=SimulationTick()-startTime;//qilu 10/22
                               //startTime = SimulationTick();//qilu 10/22
             
				               j = i + 1;
                               break;
			             } else {
                      //Return this unfound-food position to the list
                            newFoodList.push_back(LoopFunctions->FoodList[i]);
                            newFoodColoringList.push_back(LoopFunctions->FoodColoringList[i]);
                         }
                 }
      }
      if(j>0){
          for(; j < LoopFunctions->FoodList.size(); j++) {
              newFoodList.push_back(LoopFunctions->FoodList[j]);
              newFoodColoringList.push_back(LoopFunctions->FoodColoringList[j]);
          }
      }
   
      // We picked up food. Update the food list minus what we picked up.
      if(IsHoldingFood()) {
         SetClosestNest();//qilu 07/26/2016
         SetIsHeadingToNest(true);
         //SetTarget(LoopFunctions->NestPosition);
         SetTarget(ClosestNest->GetLocation()); //qilu 07/26/2016
         LoopFunctions->FoodList = newFoodList;
         LoopFunctions->FoodColoringList = newFoodColoringList; //qilu 09/12/2016
         SetLocalResourceDensity();
      }
	}
		
	// This shouldn't be checked here ---
	// Drop off food: We are holding food and have reached the nest.
	//else if((GetPosition() - LoopFunctions->NestPosition).SquareLength() < LoopFunctions->NestRadiusSquared) {
	//    isHoldingFood = false;
	// }

	// We are carrying food and haven't reached the nest, keep building up the
	// pheromone trail attached to this found food item.
 /* if(IsHoldingFood() && SimulationTick() % LoopFunctions->DrawDensityRate == 0) {
        TrailToShare.push_back(GetPosition());
  }*/
}

/*****
 * If the robot has just picked up a food item, this function will be called
 * so that the food density in the local region is analyzed and saved. This
 * helps facilitate calculations for pheromone laying.
 *
 * Ideally, given that: [*] is food, and [=] is a robot
 *
 * [*] [*] [*] | The maximum resource density that should be calculated is
 * [*] [=] [*] | equal to 9, counting the food that the robot just picked up
 * [*] [*] [*] | and up to 8 of its neighbors.
 *
 * That being said, the random and non-grid nature of movement will not
 * produce the ideal result most of the time. This is especially true since
 * item detection is based on distance calculations with circles.
 *****/
void CPFA_controller::SetLocalResourceDensity() {
	argos::CVector2 distance;

	// remember: the food we picked up is removed from the foodList before this function call
	// therefore compensate here by counting that food (which we want to count)
	ResourceDensity = 1;

	/* Calculate resource density based on the global food list positions. */
	for(size_t i = 0; i < LoopFunctions->FoodList.size(); i++) {
		   distance = GetPosition() - LoopFunctions->FoodList[i];

		   if(distance.SquareLength() < LoopFunctions->SearchRadiusSquared*2) {
			      ResourceDensity++;
			      LoopFunctions->FoodColoringList[i] = argos::CColor::ORANGE;
			      LoopFunctions->ResourceDensityDelay = SimulationTick() + SimulationTicksPerSecond() * 10;
		   }
	}
 
	/* Set the fidelity position to the robot's current position. */
    SiteFidelityPosition = GetPosition();
    isUsingSiteFidelity = true;
    updateFidelity = true; //qilu 09/07/2016
    TrailToShare.push_back(SiteFidelityPosition);//qilu 09/07/2016
    //ClosestNest->FidelityList[controllerID] = GetPosition(); //qilu 09/07/2016
    //LOG<<"ClosestNest->FidelityList["<<controllerID<<"]="<<endl;
    //ClosestNest->FidelityList.size()<<endl;
    ClosestNest->FidelityList[controllerID] = SiteFidelityPosition; //qilu 09/07/2016
    ClosestNest->DensityOnFidelity[controllerID]= ResourceDensity; //09/11/2016
	/* Delay for 4 seconds (simulate iAnts scannning rotation). */
	//  Wait(4); // This function is broken. It causes the rover to move in the wrong direction after finishing its local resource density test 

	//ofstream log_output_stream;
	//log_output_stream.open("cpfa_log.txt", ios::app);
	//log_output_stream << "(Survey): " << ResourceDensity << endl;
	//log_output_stream << "SiteFidelityPosition: " << SiteFidelityPosition << endl;
	//log_output_stream.close();
}

/*****
 * Update the global site fidelity list for graphics display and add a new fidelity position.
 *****/
void CPFA_controller::SetFidelityList(argos::CVector2 newFidelity) {
	std::vector<argos::CVector2> newFidelityList;

	/* Remove this robot's old fidelity position from the fidelity list. */
	/*for(size_t i = 0; i < LoopFunctions->FidelityList.size(); i++) {
  if((LoopFunctions->FidelityList[i] - SiteFidelityPosition).SquareLength() != 0.0) {
			newFidelityList.push_back(LoopFunctions->FidelityList[i]);
		}
	} */


	/* Update the global fidelity list. */
	//LoopFunctions->FidelityList = newFidelityList;

 ClosestNest->FidelityList[controllerID] = newFidelity; //qilu 09/08/2016
ClosestNest->DensityOnFidelity[controllerID]= ResourceDensity; //09/11/2016
	/* Add the robot's new fidelity position to the global fidelity list. */
	//LoopFunctions->FidelityList.push_back(newFidelity);
 

	/* Update the local fidelity position for this robot. */
	SiteFidelityPosition = newFidelity;
 
  updateFidelity = true;
}

/*****
 * Update the global site fidelity list for graphics display and remove the old fidelity position.
 *****/
void CPFA_controller::SetFidelityList() {
	std::vector<argos::CVector2> newFidelityList;

	/* Remove this robot's old fidelity position from the fidelity list. */
	/*for(size_t i = 0; i < LoopFunctions->FidelityList.size(); i++) {
		if((LoopFunctions->FidelityList[i] - SiteFidelityPosition).SquareLength() != 0.0) {
			newFidelityList.push_back(LoopFunctions->FidelityList[i]);
		}*/
	/* Update the global fidelity list. */
	//LoopFunctions->FidelityList = newFidelityList;
 ClosestNest->FidelityList.erase(controllerID); //qilu 09/08/2016
    ClosestNest->DensityOnFidelity.erase(controllerID); //09/11/2016
 SiteFidelityPosition = CVector2(10000, 10000);
 updateFidelity = true; 
}

/*****
 * Update the pheromone list and set the target to a pheromone position.
 * return TRUE:  pheromone was successfully targeted
 *        FALSE: pheromones don't exist or are all inactive
 *****/
bool CPFA_controller::SetTargetPheromone() {
	argos::Real maxStrength = 0.0, randomWeight = 0.0;
	bool isPheromoneSet = false;

 if(ClosestNest->PheromoneList.size()==0) return isPheromoneSet; //qilu 09/08/2016 the case of no pheromone.
	/* update the pheromone list and remove inactive pheromones */
	// LoopFunctions->UpdatePheromoneList();

	/* default target = nest; in case we have 0 active pheromones */
	SetIsHeadingToNest(true);
	//SetTarget(LoopFunctions->NestPosition);
 SetTarget(ClosestNest->GetLocation()); //qilu 07/26/2016

	/* Calculate a maximum strength based on active pheromone weights. */
	for(size_t i = 0; i < ClosestNest->PheromoneList.size(); i++) {
		if(ClosestNest->PheromoneList[i].IsActive()) {
			maxStrength += ClosestNest->PheromoneList[i].GetWeight();
		}
	}

	/* Calculate a random weight. */
	randomWeight = RNG->Uniform(argos::CRange<argos::Real>(0.0, maxStrength));

	/* Randomly select an active pheromone to follow. */
	for(size_t i = 0; i < ClosestNest->PheromoneList.size(); i++) {
		   if(randomWeight < ClosestNest->PheromoneList[i].GetWeight()) {
			       /* We've chosen a pheromone! */
			       SetIsHeadingToNest(false);
          SetTarget(ClosestNest->PheromoneList[i].GetLocation());
          TrailToFollow = ClosestNest->PheromoneList[i].GetTrail();
          isPheromoneSet = true;
          /* If we pick a pheromone, break out of this loop. */
          break;
     }

     /* We didn't pick a pheromone! Remove its weight from randomWeight. */
     randomWeight -= ClosestNest->PheromoneList[i].GetWeight();
	}

	//ofstream log_output_stream;
	//log_output_stream.open("cpfa_log.txt", ios::app);
	//log_output_stream << "Found: " << LoopFunctions->PheromoneList.size()  << " waypoints." << endl;
	//log_output_stream << "Follow waypoint?: " << isPheromoneSet << endl;
	//log_output_stream.close();

	return isPheromoneSet;
}

/*****
 * Calculate and return the exponential decay of "value."
 *****/
argos::Real CPFA_controller::GetExponentialDecay(argos::Real w, argos::Real time, argos::Real lambda) {
	/* convert time into units of haLoopFunctions-seconds from simulation frames */
	//time = time / (LoopFunctions->TicksPerSecond / 2.0);

	//LOG << "time: " << time << endl;
	//LOG << "correlation: " << (value * exp(-lambda * time)) << endl << endl;

	//return (value * std::exp(-lambda * time));
    Real     twoPi       = (CRadians::TWO_PI).GetValue();
    return w + (twoPi-w)* exp(-lambda * time);
}

/*****
 * Provides a bound on the value by rolling over a la modulo.
 *****/
argos::Real CPFA_controller::GetBound(argos::Real value, argos::Real min, argos::Real max) {
	/* Calculate an offset. */
	argos::Real offset = std::abs(min) + std::abs(max);

	/* Increment value by the offset while it's less than min. */
	while (value < min) {
			value += offset;
	}

	/* Decrement value by the offset while it's greater than max. */
	while (value > max) {
			value -= offset;
	}

	/* Return the bounded value. */
	return value;
}

size_t CPFA_controller::GetSearchingTime(){//qilu 10/22
    return searchingTime;
}
size_t CPFA_controller::GetTravelingTime(){//qilu 10/22
    return travelingTime;
}

string CPFA_controller::GetStatus(){//qilu 10/22
    //DEPARTING, SEARCHING, RETURNING
    if (MPFA_state == DEPARTING) return "DEPARTING";
    else if (MPFA_state ==SEARCHING)return "SEARCHING";
    else if (MPFA_state == RETURNING)return "RETURNING";
    else if (MPFA_state == SURVEYING) return "SURVEYING";
    //else if (MPFA_state == INACTIVE) return "INACTIVE";
    else return "SHUTDOWN";
    
}

/*void CPFA_controller::AddTravelingTime(size_t remainderTime){//qilu 10/22
    travelingTime += remainderTime;
}
void CPFA_controller::AddSearchingTime(size_t remainderTime){//qilu 10/22
    searchingTime += remainderTime;
}*/

/*****
 * Return the Poisson cumulative probability at a given k and lambda.
 *****/
argos::Real CPFA_controller::GetPoissonCDF(argos::Real k, argos::Real lambda) {
	argos::Real sumAccumulator       = 1.0;
	argos::Real factorialAccumulator = 1.0;

	for (size_t i = 1; i <= floor(k); i++) {
		factorialAccumulator *= i;
		sumAccumulator += pow(lambda, i) / factorialAccumulator;
	}

	return (exp(-lambda) * sumAccumulator);
}

void CPFA_controller::UpdateTargetRayList() {
	if(SimulationTick() % LoopFunctions->DrawDensityRate == 0 && LoopFunctions->DrawTargetRays == 1) {
		/* Get position values required to construct a new ray */
		argos::CVector2 t(GetTarget());
		argos::CVector2 p(GetPosition());
		argos::CVector3 position3d(p.GetX(), p.GetY(), 0.02);
		argos::CVector3 target3d(t.GetX(), t.GetY(), 0.02);

		/* scale the target ray to be <= searchStepSize */
		argos::Real length = std::abs(t.Length() - p.Length());

		if(length > SearchStepSize) {
			MyTrail.clear();
		} else {
			/* add the ray to the robot's target trail */
			argos::CRay3 targetRay(target3d, position3d);
			MyTrail.push_back(targetRay);

			/* delete the oldest ray from the trail */
			if(MyTrail.size() > MaxTrailSize) {
				MyTrail.erase(MyTrail.begin());
			}

			LoopFunctions->TargetRayList.insert(LoopFunctions->TargetRayList.end(), MyTrail.begin(), MyTrail.end());
			// loopFunctions.TargetRayList.push_back(myTrail);
		}
	}
}

void CPFA_controller::SetClosestNest(){//qilu 07/26/2016
    Nest* NewClosestNest;
    CVector2 robotPos = GetPosition();
    Real minSquaredLen = (LoopFunctions->Nests[0].GetLocation()-robotPos).SquareLength();
    size_t minIdex =0;
    Real squaredLen;
    for(size_t i=1; i<LoopFunctions->Nests.size(); i++){
        squaredLen = (LoopFunctions->Nests[i].GetLocation()-robotPos).SquareLength();
        if (squaredLen < minSquaredLen) {
            minSquaredLen = squaredLen;
            minIdex = i;
        }
    }
    //ClosestNest->FidelityList.erase(controllerID); //qilu 09/07/2016
		  //SiteFidelityPosition=CVector2(10000,10000);
		
    NewClosestNest = &LoopFunctions->Nests[minIdex];
    if(ClosestNest != NULL){
        if(ClosestNest->GetNestIdx() != NewClosestNest->GetNestIdx())
            SetFidelityList();
            TrailToShare.clear();//qilu 09/29/2016
            ClosestNest = NewClosestNest;
    }
    else ClosestNest = NewClosestNest;
}
REGISTER_CONTROLLER(CPFA_controller, "CPFA_controller")
