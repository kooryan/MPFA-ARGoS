#include "CPFA_loop_functions.h"

CPFA_loop_functions::CPFA_loop_functions() :
	RNG(argos::CRandom::CreateRNG("argos")),
        SimTime(0), //qilu 09/13/2016
	MaxSimTime(3600 * GetSimulator().GetPhysicsEngine("dyn2d").GetInverseSimulationClockTick()),
	CollisionTime(0),
	currNumCollectedFood(0), //qilu 08/19
        lastNumCollectedFood(0), //qilu 08/19
	ResourceDensityDelay(0),
	RandomSeed(GetSimulator().GetRandomSeed()),
	SimCounter(0),
	MaxSimCounter(1),
	VariableFoodPlacement(0),
	OutputData(0),
	DrawDensityRate(4),
	DrawIDs(1),
	DrawTrails(1),
	DrawTargetRays(1),
	FoodDistribution(2),
	FoodItemCount(256),
	NumberOfClusters(4),
	ClusterWidthX(8),
	ClusterLengthY(8),
	PowerRank(4),
	PowerLawCopies(1),
	ProbabilityOfSwitchingToSearching(0.0),
	ProbabilityOfReturningToNest(0.0),
	UninformedSearchVariation(0.0),
	RateOfInformedSearchDecay(0.0),
	RateOfSiteFidelity(0.0),
	RateOfLayingPheromone(0.0),
	RateOfPheromoneDecay(0.0),
	FoodRadius(0.05),
	FoodRadiusSquared(0.0025),
	NestRadius(0.25),
	NestRadiusSquared(0.0625),
	NestElevation(0.01),
	// We are looking at a 4 by 4 square (3 targets + 2*1/2 target gaps)
	SearchRadiusSquared((4.0 * FoodRadius) * (4.0 * FoodRadius)),
	NumDistributedFood(0),
	score(0),
	PrintFinalScore(0)
{}

void CPFA_loop_functions::Init(argos::TConfigurationNode &node) {	
 CVector2		NestPosition; //qilu 09/06
 
	argos::CDegrees USV_InDegrees;
	argos::TConfigurationNode CPFA_node = argos::GetNode(node, "CPFA");

	argos::GetNodeAttribute(CPFA_node, "ProbabilityOfSwitchingToSearching", ProbabilityOfSwitchingToSearching);
	argos::GetNodeAttribute(CPFA_node, "ProbabilityOfReturningToNest",      ProbabilityOfReturningToNest);
	argos::GetNodeAttribute(CPFA_node, "UninformedSearchVariation",         USV_InDegrees);
	argos::GetNodeAttribute(CPFA_node, "RateOfInformedSearchDecay",         RateOfInformedSearchDecay);
	argos::GetNodeAttribute(CPFA_node, "RateOfSiteFidelity",                RateOfSiteFidelity);
	argos::GetNodeAttribute(CPFA_node, "RateOfLayingPheromone",             RateOfLayingPheromone);
	argos::GetNodeAttribute(CPFA_node, "RateOfPheromoneDecay",              RateOfPheromoneDecay);
	argos::GetNodeAttribute(CPFA_node, "PrintFinalScore",                   PrintFinalScore);

	UninformedSearchVariation = ToRadians(USV_InDegrees);
	argos::TConfigurationNode settings_node = argos::GetNode(node, "settings");

	argos::GetNodeAttribute(settings_node, "MaxSimTimeInSeconds", MaxSimTime);

	MaxSimTime *= GetSimulator().GetPhysicsEngine("dyn2d").GetInverseSimulationClockTick();

	argos::GetNodeAttribute(settings_node, "MaxSimCounter", MaxSimCounter);
	argos::GetNodeAttribute(settings_node, "VariableFoodPlacement", VariableFoodPlacement);
	argos::GetNodeAttribute(settings_node, "OutputData", OutputData);
	argos::GetNodeAttribute(settings_node, "DrawIDs", DrawIDs);
	argos::GetNodeAttribute(settings_node, "DrawTrails", DrawTrails);
	argos::GetNodeAttribute(settings_node, "DrawTargetRays", DrawTargetRays);
	argos::GetNodeAttribute(settings_node, "FoodDistribution", FoodDistribution);
	argos::GetNodeAttribute(settings_node, "FoodItemCount", FoodItemCount);
	argos::GetNodeAttribute(settings_node, "NumberOfClusters", NumberOfClusters);
	argos::GetNodeAttribute(settings_node, "ClusterWidthX", ClusterWidthX);
	argos::GetNodeAttribute(settings_node, "ClusterLengthY", ClusterLengthY);
	argos::GetNodeAttribute(settings_node, "PowerRank", PowerRank);
	argos::GetNodeAttribute(settings_node, "PowerLawCopies", PowerLawCopies);
	argos::GetNodeAttribute(settings_node, "FoodRadius", FoodRadius);
        argos::GetNodeAttribute(settings_node, "NestRadius", NestRadius); //qilu 09/12/2016
	argos::GetNodeAttribute(settings_node, "NestElevation", NestElevation);
	argos::GetNodeAttribute(settings_node, "ArenaWidth", ArenaWidth);//qilu 11/06/2016
 argos::GetNodeAttribute(settings_node, "NestPosition_0", NestPosition);
 Nest nest0= Nest(NestPosition); //qilu 09/06
 nest0.SetNestIdx(0);
 Nests.push_back(nest0);
    
    
 argos::GetNodeAttribute(settings_node, "NestPosition_1", NestPosition);
 Nest nest1= Nest(NestPosition); //qilu 09/06
 nest1.SetNestIdx(1);
 Nests.push_back(nest1);
 
 argos::GetNodeAttribute(settings_node, "NestPosition_2", NestPosition);
 Nest nest2= Nest(NestPosition); //qilu 09/06
 nest2.SetNestIdx(2);
 Nests.push_back(nest2);
 
 
 argos::GetNodeAttribute(settings_node, "NestPosition_3", NestPosition);
 Nest nest3= Nest(NestPosition); //qilu 09/06
 nest3.SetNestIdx(3);
 Nests.push_back(nest3);
 
 CCylinderEntity& cCyl1 =
   dynamic_cast<CCylinderEntity&>(GetSpace().GetEntity("cyl1"));
Cylinders.push_back(cCyl1);

CCylinderEntity& cCyl2 =
   dynamic_cast<CCylinderEntity&>(GetSpace().GetEntity("cyl2"));
Cylinders.push_back(cCyl2);

CCylinderEntity& cCyl3 =
   dynamic_cast<CCylinderEntity&>(GetSpace().GetEntity("cyl3"));
Cylinders.push_back(cCyl3);

CCylinderEntity& cCyl4 =
   dynamic_cast<CCylinderEntity&>(GetSpace().GetEntity("cyl4"));
Cylinders.push_back(cCyl4);

	FoodRadiusSquared = FoodRadius*FoodRadius;

        //Number of distributed foods
    if (FoodDistribution == 0){
        NumDistributedFood = FoodItemCount; 
    }
    else if (FoodDistribution == 1){
        NumDistributedFood = ClusterWidthX*ClusterLengthY*NumberOfClusters;
    }
    else{
        for(size_t i=0; i<PowerRank; i++)
	    NumDistributedFood += pow(4, PowerRank-1);
	NumDistributedFood *= PowerLawCopies;     
    }

	// calculate the forage range and compensate for the robot's radius of 0.085m
	argos::CVector3 ArenaSize = GetSpace().GetArenaSize();
	argos::Real rangeX = (ArenaSize.GetX() / 2.0) - 0.085;
	argos::Real rangeY = (ArenaSize.GetY() / 2.0) - 0.085;
	ForageRangeX.Set(-rangeX, rangeX);
	ForageRangeY.Set(-rangeY, rangeY);

	   // Send a pointer to this loop functions object to each controller.
	   argos::CSpace::TMapPerType& footbots = GetSpace().GetEntitiesByType("foot-bot");
	   argos::CSpace::TMapPerType::iterator it;
    Num_robots = footbots.size();
    for(it = footbots.begin(); it != footbots.end(); it++) {
   	argos::CFootBotEntity& footBot = *argos::any_cast<argos::CFootBotEntity*>(it->second);
        BaseController& c = dynamic_cast<BaseController&>(footBot.GetControllableEntity().GetController());
	CPFA_controller& c2 = dynamic_cast<CPFA_controller&>(c);
        c2.SetLoopFunctions(this);
	    }
     
     /*CSpace::TMapPerType& theMap = GetSpace().GetEntitiesByType("box"); 
     argos::CBoxEntity* box = any_cast<CBoxEntity*>(theMap["my_box"]); 
     LOG<<box.GetId()<<endl;
     */
      /*argos::CSpace::TMapPerType& theCylinder = GetSpace().GetEntitiesByType("cylinder");
      argos::CCylinderEntity* cylinder1 = any_cast<CCylinderEntity*>(theCylinder["cyl1"]);
      
      LOG<<cylinder1.GetId()<<endl; */ 
    /*argos::CSpace::TMapPerType& cylinders = GetSpace().GetEntitiesByType("cylinder");
    argos::CSpace::TMapPerType::iterator tt;
    for(tt = cylinders.begin(); tt != cylinders.end(); tt++){
        CCylinderEntity& cylinder = *argos::any_cast<CCylinderEntity*>(tt->second);
		      LOG<<"cylinder.id = "<<tt.GetId()<<endl;
     
     }*/
     

    SetFoodDistribution();
    ReallocateNests();
       
    for(size_t i=0; i<Nests.size(); i++) MoveEntity(Cylinders[i].GetEmbodiedEntity(), CVector3(Nests[i].GetLocation().GetX(), Nests[i].GetLocation().GetY(),0), CQuaternion());
    
    ForageList.clear(); //qilu 09/13/2016
 
  
    last_time_in_minutes=0; //qilu 09/13/2016
    Nest_travel_time_in_ticks=0;
    //CollisionTimeList.clear();//qilu 09/26/2016
 
}

/*vector<CVector2> CPFA_loop_functions::UpdateCollectedFoodList(vector<CVector2> foodList){//qilu 09/12/2016
    if((GetPosition() - LoopFunctions->FoodList[i]).SquareLength() < FoodDistanceTolerance )

    for (size_t i=0; i<foodList.size(); i++) {
        foodList[i]

    }
    return
    
}*/


void CPFA_loop_functions::Reset() {
	   if(VariableFoodPlacement == 0) {
		      RNG->Reset();
	   }

    GetSpace().Reset();
    GetSpace().GetFloorEntity().Reset();
    MaxSimCounter = SimCounter;
    SimCounter = 0;
   
    FoodList.clear();
    FoodColoringList.clear();
   
    TargetRayList.clear();
    
    SetFoodDistribution();
    for(size_t i=0; i<Nests.size(); i++){ //qilu 09/06
      Nests[i].FidelityList.clear();
      Nests[i].DensityOnFidelity.clear(); //qilu 09/11/2016
      Nests[i].PheromoneList.clear();
      }
    
    argos::CSpace::TMapPerType& footbots = GetSpace().GetEntitiesByType("foot-bot");
    
    Num_robots = footbots.size();
    
    for(argos::CSpace::TMapPerType::iterator it = footbots.begin(); it != footbots.end(); it++) {
        argos::CFootBotEntity& footBot = *argos::any_cast<argos::CFootBotEntity*>(it->second);
        BaseController& c = dynamic_cast<BaseController&>(footBot.GetControllableEntity().GetController());
        CPFA_controller& c2 = dynamic_cast<CPFA_controller&>(c);
        MoveEntity(footBot.GetEmbodiedEntity(), c2.GetStartPosition(), argos::CQuaternion(), false);
    }
}

void CPFA_loop_functions::PreStep() {
    SimTime++;
    curr_time_in_minutes = getSimTimeInSeconds()/60.0;
    if(curr_time_in_minutes - last_time_in_minutes==1){
		      //CollisionTimeList.push_back(currCollisionTime - lastCollisionTime);
		      //lastCollisionTime = currCollisionTime;
				
        ForageList.push_back(currNumCollectedFood - lastNumCollectedFood);
        lastNumCollectedFood = currNumCollectedFood;
        last_time_in_minutes++;
    }
    CVector2 offset, newLocation;
    
    for(size_t i=0; i<Nests.size(); i++){
        if(Nests[i].GetLocation() != Nests[i].NewLocation){
            offset = (Nests[i].NewLocation - Nests[i].GetLocation()).Normalize();
            offset *= 0.001;
	        newLocation = Nests[i].GetLocation()+offset;
            Nests[i].SetLocation(newLocation);
            MoveEntity(Cylinders[i].GetEmbodiedEntity(), CVector3(newLocation.GetX(),newLocation.GetY(),0), CQuaternion());
            Nest_travel_time_in_ticks++;
         }
    }
    
	UpdatePheromoneList();

	if(GetSpace().GetSimulationClock() > ResourceDensityDelay) {
     for(size_t i = 0; i < FoodColoringList.size(); i++) {
         FoodColoringList[i] = argos::CColor::BLACK;
     }
	}
 
    if(FoodList.size() == 0) {
        for(size_t i=0; i<Nests.size(); i++){
            Nests[i].PheromoneList.clear();//qilu 09/11/16
            Nests[i].FidelityList.clear();//qilu 09/11/16
            Nests[i].DensityOnFidelity.clear(); //qilu 09/11/2016
        }
        TargetRayList.clear();//qilu 07/13 
    }
}

void CPFA_loop_functions::PostStep() {
	// nothing... yet...
}

bool CPFA_loop_functions::IsExperimentFinished() {
	bool isFinished = false;

	if(FoodList.size() == 0 || GetSpace().GetSimulationClock() >= MaxSimTime) {
		isFinished = true;
	}
        
        //set to collected 88% food and then stop
        if(score >= NumDistributedFood*0.8){
		isFinished = true;
	}

	if(isFinished == true && MaxSimCounter > 1) {
		size_t newSimCounter = SimCounter + 1;
		size_t newMaxSimCounter = MaxSimCounter - 1;

		PostExperiment();
		Reset();

		SimCounter    = newSimCounter;
		MaxSimCounter = newMaxSimCounter;
		isFinished    = false;
	}

	return isFinished;
}

void CPFA_loop_functions::PostExperiment() {

    
	   printf("%f, %f, %lu\n", score, getSimTimeInSeconds(), RandomSeed);
        
                
    if (PrintFinalScore == 1) {
        string type="";
        if (FoodDistribution == 0) type = "random";
        else if (FoodDistribution == 1) type = "cluster";
        else type = "powerlaw";

        ostringstream num_tag_str;
        num_tag_str << NumDistributedFood; 
        
	        
        ostringstream num_nests_str;
        num_nests_str <<  Nests.size();
    
        ostringstream num_robots_str;
        num_robots_str <<  Num_robots;
   
        ostringstream arena_width_str;
        arena_width_str << ArenaWidth;
   
        string header = type+"_dynamic_K-means_n"+num_nests_str.str()+"_r"+num_robots_str.str()+"_tag"+num_tag_str.str()+"_"+arena_width_str.str()+"by"+arena_width_str.str()+"_";
       
        Real total_travel_time=0;
        Real total_search_time=0;
        
        ofstream travelSearchTimeDataOutput((header+"TravelSearchTimeData.txt").c_str(), ios::app);
        //travelSearchTimeDataOutput<<"Traveling time, Searching time"<<endl;
        
        unsigned int ticks_per_second = GetSimulator().GetPhysicsEngine("Default").GetInverseSimulationClockTick();

        argos::CSpace::TMapPerType& footbots = GetSpace().GetEntitiesByType("foot-bot");
         
        for(argos::CSpace::TMapPerType::iterator it = footbots.begin(); it != footbots.end(); it++) {
            argos::CFootBotEntity& footBot = *argos::any_cast<argos::CFootBotEntity*>(it->second);
            BaseController& c = dynamic_cast<BaseController&>(footBot.GetControllableEntity().GetController());
            CPFA_controller& c2 = dynamic_cast<CPFA_controller&>(c);
            CollisionTime += c2.GetCollisionTime();
            
            if(c2.GetStatus() == "SEARCHING"){
                total_search_time += SimTime-c2.GetTravelingTime();
                total_travel_time += c2.GetTravelingTime();
            }
            else{
                total_search_time += c2.GetSearchingTime();
                total_travel_time += SimTime-c2.GetSearchingTime();
            }
        }
        travelSearchTimeDataOutput<< total_travel_time/ticks_per_second<<", "<<total_search_time/ticks_per_second<<endl;
        travelSearchTimeDataOutput.close();    
               
        ofstream dataOutput( (header+ "iAntTagData.txt").c_str(), ios::app);
        // output to file
        if(dataOutput.tellp() == 0) {
            dataOutput << "tags_collected, collisions_in_seconds, time_in_minutes, random_seed\n";//qilu 08/18
        }
        dataOutput << Score() << ", "<<(CollisionTime-16*Score())/(2*ticks_per_second)<< ", "<< curr_time_in_minutes <<", "<<RandomSeed<<endl;
        dataOutput.close();
    
        ofstream forageDataOutput((header+"ForageData.txt").c_str(), ios::app);
        if(ForageList.size()!=0) forageDataOutput<<"Forage: "<< ForageList[0];
            for(size_t i=1; i< ForageList.size(); i++) forageDataOutput<<", "<<ForageList[i];
            forageDataOutput<<"\n";
            forageDataOutput.close();
     
      }  

}


argos::CColor CPFA_loop_functions::GetFloorColor(const argos::CVector2 &c_pos_on_floor) {
	return argos::CColor::WHITE;
}

void CPFA_loop_functions::UpdatePheromoneList() {
	// Return if this is not a tick that lands on a 0.5 second interval
	if ((int)(GetSpace().GetSimulationClock()) % ((int)(GetSimulator().GetPhysicsEngine("dyn2d").GetInverseSimulationClockTick()) / 2) != 0) return;
	
	std::vector<Pheromone> new_p_list; 

	argos::Real t = GetSpace().GetSimulationClock() / GetSimulator().GetPhysicsEngine("dyn2d").GetInverseSimulationClockTick();

	//ofstream log_output_stream;
	//log_output_stream.open("time.txt", ios::app);
	//log_output_stream << t << ", " << GetSpace().GetSimulationClock() << ", " << GetSimulator().GetPhysicsEngine("default").GetInverseSimulationClockTick() << endl;
	//log_output_stream.close();
 for(size_t n=0; n<Nests.size();n++){
	    for(size_t i = 0; i < Nests[n].PheromoneList.size(); i++) {

		        //Nests[n].PheromoneList[i].Update(t);
                Nests[n].PheromoneList[i].Update(getSimTimeInSeconds());//qilu 09/24/2016 let it decays in every second
		        if(Nests[n].PheromoneList[i].IsActive()) {
			          new_p_list.push_back(Nests[n].PheromoneList[i]);
		        }
      }
     	Nests[n].PheromoneList = new_p_list;
      new_p_list.clear();//qilu 09/08/2016
 }
}
void CPFA_loop_functions::SetFoodDistribution() {
	switch(FoodDistribution) {
		case 0:
			RandomFoodDistribution();
			break;
		case 1:
			ClusterFoodDistribution();
			break;
		case 2:
			PowerLawFoodDistribution();
			break;
  case 3:
			GaussianFoodDistribution();
			break;
		default:
			argos::LOGERR << "ERROR: Invalid food distribution in XML file.\n";
	}
}

void CPFA_loop_functions::RandomFoodDistribution() {
	FoodList.clear();
    FoodColoringList.clear();
    FoodClusterIDs.clear();
    FoodDistances.clear();

	argos::CVector2 placementPosition;

	for(size_t i = 0; i < FoodItemCount; i++) {
		placementPosition.Set(RNG->Uniform(ForageRangeX), RNG->Uniform(ForageRangeY));

		while(IsOutOfBounds(placementPosition, 1, 1)) {
			placementPosition.Set(RNG->Uniform(ForageRangeX), RNG->Uniform(ForageRangeY));
		}

		FoodList.push_back(placementPosition);
		FoodColoringList.push_back(argos::CColor::BLACK);
		FoodClusterIDs.push_back(0);
		FoodDistances.push_back(10000);
	}
}

void CPFA_loop_functions::GaussianFoodDistribution() {
 FoodList.clear();
 FoodColoringList.clear();
 argos::CVector2 centerPosition;
 argos::CVector2 placementPosition;
 
 for(size_t i = 0; i < NumberOfClusters; i++) {
		  centerPosition.Set(RNG->Uniform(ForageRangeX), RNG->Uniform(ForageRangeY));

		  while(IsOutOfArena(centerPosition) || IsCollidingWithNest(centerPosition, NestRadius)) {
			  centerPosition.Set(RNG->Uniform(ForageRangeX), RNG->Uniform(ForageRangeY));
		  }
    for(size_t i=0; i<32; i++){
       placementPosition.Set(centerPosition.GetX() + RNG->Gaussian(0.15, 0), centerPosition.GetY() + RNG->Gaussian(0.15, 0));
       while(IsOutOfArena(placementPosition, FoodRadius)){
            placementPosition.Set(centerPosition.GetX() + RNG->Gaussian(0.15, 0), centerPosition.GetY() + RNG->Gaussian(0.15, 0));
        }
       
       FoodList.push_back(placementPosition);
       FoodColoringList.push_back(argos::CColor::BLACK);
       FoodClusterIDs.push_back(0);
	   FoodDistances.push_back(10000);
     }
   }
 }
 
void CPFA_loop_functions::ClusterFoodDistribution() {
    FoodList.clear();
    FoodColoringList.clear();
	argos::Real     foodOffset  = 3.0 * FoodRadius;
	size_t          foodToPlace = NumberOfClusters * ClusterWidthX * ClusterLengthY;
	size_t          foodPlaced = 0;
	argos::CVector2 placementPosition;

	FoodItemCount = foodToPlace;

	for(size_t i = 0; i < NumberOfClusters; i++) {
		placementPosition.Set(RNG->Uniform(ForageRangeX), RNG->Uniform(ForageRangeY));

		while(IsOutOfBounds(placementPosition, ClusterLengthY, ClusterWidthX)) {
			placementPosition.Set(RNG->Uniform(ForageRangeX), RNG->Uniform(ForageRangeY));
		}

		for(size_t j = 0; j < ClusterLengthY; j++) {
			for(size_t k = 0; k < ClusterWidthX; k++) {
				foodPlaced++;
				/*
				#include <argos3/plugins/simulator/entities/box_entity.h>

				string label("my_box_");
				label.push_back('0' + foodPlaced++);

				CBoxEntity *b = new CBoxEntity(label,
					CVector3(placementPosition.GetX(),
					placementPosition.GetY(), 0.0), CQuaternion(), true,
					CVector3(0.1, 0.1, 0.001), 1.0);
				AddEntity(*b);
				*/

				FoodList.push_back(placementPosition);
				FoodColoringList.push_back(argos::CColor::BLACK);
				FoodClusterIDs.push_back(0);
		        FoodDistances.push_back(10000);
				placementPosition.SetX(placementPosition.GetX() + foodOffset);
			}

			placementPosition.SetX(placementPosition.GetX() - (ClusterWidthX * foodOffset));
			placementPosition.SetY(placementPosition.GetY() + foodOffset);
		}
	}
}


void CPFA_loop_functions::PowerLawFoodDistribution() {
    FoodList.clear();
    FoodColoringList.clear();
    argos::Real foodOffset     = 3.0 * FoodRadius;
	size_t      foodPlaced     = 0;
	size_t      powerLawLength = 1;
	size_t      maxTrials      = 200;
	size_t      trialCount     = 0;

	std::vector<size_t> powerLawClusters;
	std::vector<size_t> clusterSides;
	argos::CVector2     placementPosition;

	for(size_t i = 0; i < PowerRank; i++) {
		powerLawClusters.push_back(powerLawLength * powerLawLength);
		powerLawLength *= 2;
	}

	for(size_t i = 0; i < PowerRank; i++) {
		powerLawLength /= 2;
		clusterSides.push_back(powerLawLength);
	}
    for(size_t c=0; c< PowerLawCopies; c++){
	for(size_t h = 0; h < powerLawClusters.size(); h++) {
		for(size_t i = 0; i < powerLawClusters[h]; i++) {
			placementPosition.Set(RNG->Uniform(ForageRangeX), RNG->Uniform(ForageRangeY));

			while(IsOutOfBounds(placementPosition, clusterSides[h], clusterSides[h])) {
				trialCount++;
				placementPosition.Set(RNG->Uniform(ForageRangeX), RNG->Uniform(ForageRangeY));

				if(trialCount > maxTrials) {
					argos::LOGERR << "PowerLawDistribution(): Max trials exceeded!\n";
					break;
				}
			}

			for(size_t j = 0; j < clusterSides[h]; j++) {
				for(size_t k = 0; k < clusterSides[h]; k++) {
					foodPlaced++;
					FoodList.push_back(placementPosition);
					FoodColoringList.push_back(argos::CColor::BLACK);
					FoodClusterIDs.push_back(0);
		            FoodDistances.push_back(10000);
					placementPosition.SetX(placementPosition.GetX() + foodOffset);
				}

				placementPosition.SetX(placementPosition.GetX() - (clusterSides[h] * foodOffset));
				placementPosition.SetY(placementPosition.GetY() + foodOffset);
			}
		}
	}
    }
	FoodItemCount = foodPlaced;
}

void CPFA_loop_functions::ReallocateNests(){
    vector<CVector2> centers; 
    centers = ComputeNewCenters();//select centers by the initialization of K-means++ 
    Centroidal_Voronoi_tessellation(centers);
}
void CPFA_loop_functions::Centroidal_Voronoi_tessellation(vector<CVector2> centers){
	int counter = 0; //check whether it is convergent
    vector<CVector2> newCenters;
    while (counter != Nests.size()) {
	counter = 0;
        newCenters = Clustering(centers);
        for(size_t i=0; i<Nests.size(); i++){
	    if((centers[i] - newCenters[i]).SquareLength() < 0.001) counter++;
	}
        centers = newCenters;
        
    }
    for(size_t i=0; i<Nests.size(); i++){ 
        Nests[i].SetLocation(newCenters[i]);
        Nests[i].NewLocation = newCenters[i];
	}
    ClusterColorList.clear();
    for (size_t i=0; i<FoodList.size(); i++) {
        if(FoodClusterIDs[i] == 0)
            ClusterColorList.push_back(CColor::YELLOW);
        else if(FoodClusterIDs[i] == 1)
            ClusterColorList.push_back(CColor::GREEN);
        else if(FoodClusterIDs[i] == 2)
            ClusterColorList.push_back(CColor::MAGENTA);
        else if(FoodClusterIDs[i] == 3)
            ClusterColorList.push_back(CColor::RED);
    }
}
vector<CVector2> CPFA_loop_functions::ComputeNewCenters(){
    int point_idx = ((int)rand()% FoodList.size()); //select the first center randomly
    double distance;
    double sum_distance=0.0;
    double selected_value;

    vector<CVector2> cluster_centers;
    cluster_centers.push_back(FoodList[point_idx]);
    while (cluster_centers.size() != Nests.size()) {
        //Compute the distance to the closest center
        for (int i=0; i<cluster_centers.size(); i++) {
            for (int j=0; j<FoodList.size(); j++) {
                distance = (cluster_centers[i] - FoodList[j]).SquareLength();
                if (distance < FoodDistances[j]) {
                    FoodDistances[j] = distance;
                    FoodClusterIDs[j] = i;
                }
            }
        }
        //select a new center
        for (size_t i=0; i<FoodList.size(); i++)
            sum_distance += FoodDistances[i];
        selected_value = (((int)rand() % 100)/100.0) * sum_distance;
        for (size_t i=0; i<FoodList.size(); i++) {
            selected_value -= FoodDistances[i];
            if (selected_value>0) continue;
            cluster_centers.push_back(FoodList[i]);
            break;
        }
        sum_distance =0.0;
    }
    return cluster_centers;

}

vector<CVector2> CPFA_loop_functions::Clustering(vector<CVector2> cluster_centers){
    //Cluster the points based on the selected centers
    Real min_distance;
    Real distance;
    vector<CVector2> newCenters;
    vector<CVector2> sum_locations;
    vector<int> sum_nums;

    for (size_t i=0; i<FoodList.size(); i++) {
        min_distance = 10000.0;
        for (int k=0; k<cluster_centers.size(); k++) {
            distance = (cluster_centers[k]- FoodList[i]).SquareLength();
            if(min_distance > distance) {
                min_distance = distance;
                FoodClusterIDs[i] = k;
            }
        }
    }
    for (int i=0; i<Nests.size(); i++) {
        sum_locations.push_back(CVector2(0.0, 0.0));
        sum_nums.push_back(0);
    }
    //compute the new centers
    for (size_t i=0; i<FoodList.size(); i++) {
        sum_locations[FoodClusterIDs[i]] += FoodList[i];
        sum_nums[FoodClusterIDs[i]] += 1;
    }
    for (int k=0; k<sum_locations.size(); k++) {
		if(sum_nums[k] == 0)
            newCenters.push_back(CVector2(0,0));
        else
            newCenters.push_back(sum_locations[k]/sum_nums[k]);      
        }
    sum_locations.clear();
    sum_nums.clear();
    return newCenters;
}


void CPFA_loop_functions::CreateNest(argos::CVector2 position){ //qilu 07/26/2016
     Real     x_coordinate = position.GetX();
     Real     y_coordinate = position.GetY();
     size_t  num_trail=0;    
     argos::CRange<argos::Real>   RangeX;
     RangeX.Set(x_coordinate - NestRadius, x_coordinate + NestRadius);
     argos::CRange<argos::Real>   RangeY;
     RangeY.Set(y_coordinate - NestRadius, y_coordinate + NestRadius);
     while(IsOutOfBounds(position, NestRadius) && num_trail<20){
             position.Set(RNG->Uniform(RangeX), RNG->Uniform(RangeY));
             num_trail++;
      }
      Nests.push_back(Nest(position));
 }
 
 
bool CPFA_loop_functions::IsOutOfArena(argos::CVector2 p){
  argos::Real x = p.GetX();
	 argos::Real y = p.GetY();
  
  if((x < ForageRangeX.GetMin())
			|| (x > ForageRangeX.GetMax()) ||
			(y < ForageRangeY.GetMin()) ||
			(y > ForageRangeY.GetMax()))		return true;
    return false;
   }
 
 bool CPFA_loop_functions::IsOutOfArena(argos::CVector2 p, argos::Real radius){
  argos::Real x_min = p.GetX() - radius;
	 argos::Real x_max = p.GetX() + radius;

	 argos::Real y_min = p.GetY() - radius;
	 argos::Real y_max = p.GetY() + radius;
  if((x_min < (ForageRangeX.GetMin() + radius))
			|| (x_max > (ForageRangeX.GetMax() - radius)) ||
			(y_min < (ForageRangeY.GetMin() + radius)) ||
			(y_max > (ForageRangeY.GetMax() - radius)))		return true;
     return false;
   
   }
   
bool CPFA_loop_functions::IsOutOfBounds(argos::CVector2 p, argos::Real radius){ //qilu 07/26/2016
  /*
  argos::Real x_min = p.GetX() - radius;
	 argos::Real x_max = p.GetX() + radius;

	 argos::Real y_min = p.GetY() - radius;
	 argos::Real y_max = p.GetY() + radius;
  if((x_min < (ForageRangeX.GetMin() + radius))
			|| (x_max > (ForageRangeX.GetMax() - radius)) ||
			(y_min < (ForageRangeY.GetMin() + radius)) ||
			(y_max > (ForageRangeY.GetMax() - radius)))		return true;*/
  if (IsOutOfArena(p, radius)) return true;
  
  if(IsCollidingWithFood(p, radius)) return true;
  
  if(IsCollidingWithNest(p, radius)) return true;

  return false;
}
bool CPFA_loop_functions::IsOutOfBounds(argos::CVector2 p, size_t length, size_t width) {
	argos::CVector2 placementPosition = p;

	argos::Real foodOffset   = 3.0 * FoodRadius;
	argos::Real widthOffset  = 3.0 * FoodRadius * (argos::Real)width;
	argos::Real lengthOffset = 3.0 * FoodRadius * (argos::Real)length;

	argos::Real x_min = p.GetX() - FoodRadius;
	argos::Real x_max = p.GetX() + FoodRadius + widthOffset;

	argos::Real y_min = p.GetY() - FoodRadius;
	argos::Real y_max = p.GetY() + FoodRadius + lengthOffset;

	if((x_min < (ForageRangeX.GetMin() + FoodRadius))
			|| (x_max > (ForageRangeX.GetMax() - FoodRadius)) ||
			(y_min < (ForageRangeY.GetMin() + FoodRadius)) ||
			(y_max > (ForageRangeY.GetMax() - FoodRadius)))
	{
		return true;
	}

	for(size_t j = 0; j < length; j++) {
		for(size_t k = 0; k < width; k++) {
			if(IsCollidingWithFood(placementPosition)) return true;
			if(IsCollidingWithNest(placementPosition)) return true;
			placementPosition.SetX(placementPosition.GetX() + foodOffset);
		}

		placementPosition.SetX(placementPosition.GetX() - (width * foodOffset));
		placementPosition.SetY(placementPosition.GetY() + foodOffset);
	}

	return false;
}

bool CPFA_loop_functions::IsCollidingWithNest(argos::CVector2 p, argos::Real radius){ //qilu 07/26/2016 for nest
 argos::Real nestRadiusPlusBuffer = NestRadius + radius;
	argos::Real NRPB_squared = nestRadiusPlusBuffer * nestRadiusPlusBuffer;
 if(Nests.size()==0) return false; //qilu 07/26/2016
 
 for(size_t i=0; i < Nests.size(); i++){ //qilu 07/26/2016
      if( (p - Nests[i].GetLocation()).SquareLength() < NRPB_squared) return true;
  }
    return false;
 }
  
bool CPFA_loop_functions::IsCollidingWithNest(argos::CVector2 p) {
	argos::Real nestRadiusPlusBuffer = NestRadius + FoodRadius;
	argos::Real NRPB_squared = nestRadiusPlusBuffer * nestRadiusPlusBuffer;

 if(Nests.size()==0) return false; //qilu 07/26/2016
	//return ((p - NestPosition).SquareLength() < NRPB_squared);
 for(size_t i=0; i < Nests.size(); i++){ //qilu 07/26/2016
      if( (p - Nests[i].GetLocation()).SquareLength() < NRPB_squared) return true;
  }
  return false;
}

bool CPFA_loop_functions::IsCollidingWithFood(argos::CVector2 p, argos::Real radius){ //qilu 07/26/2016 for nest
 argos::Real foodRadiusPlusBuffer = FoodRadius + radius;
	argos::Real FRPB_squared = foodRadiusPlusBuffer * foodRadiusPlusBuffer;
 
 for(size_t i=0; i < FoodList.size(); i++){ //qilu 07/26/2016
      if( (p - FoodList[i]).SquareLength() < FRPB_squared) return true;
  }
    return false;
}

bool CPFA_loop_functions::IsCollidingWithFood(argos::CVector2 p) {
	argos::Real foodRadiusPlusBuffer = 2.0 * FoodRadius;
	argos::Real FRPB_squared = foodRadiusPlusBuffer * foodRadiusPlusBuffer;

	for(size_t i = 0; i < FoodList.size(); i++) {
		if((p - FoodList[i]).SquareLength() < FRPB_squared) return true;
	}

	return false;
}

unsigned int CPFA_loop_functions::getNumberOfRobots() {
	return GetSpace().GetEntitiesByType("foot-bot").size();
}

double CPFA_loop_functions::getProbabilityOfSwitchingToSearching() {
	return ProbabilityOfSwitchingToSearching;
}

double CPFA_loop_functions::getProbabilityOfReturningToNest() {
	return ProbabilityOfReturningToNest;
}

// Value in Radians
double CPFA_loop_functions::getUninformedSearchVariation() {
	return UninformedSearchVariation.GetValue();
}

double CPFA_loop_functions::getRateOfInformedSearchDecay() {
	return RateOfInformedSearchDecay;
}

double CPFA_loop_functions::getRateOfSiteFidelity() {
	return RateOfSiteFidelity;
}

double CPFA_loop_functions::getRateOfLayingPheromone() {
	return RateOfLayingPheromone;
}

double CPFA_loop_functions::getRateOfPheromoneDecay() {
	return RateOfPheromoneDecay;
}

argos::Real CPFA_loop_functions::getSimTimeInSeconds() {
	int ticks_per_second = GetSimulator().GetPhysicsEngine("Default").GetInverseSimulationClockTick();
	float sim_time = GetSpace().GetSimulationClock();
	return sim_time/ticks_per_second;
}

void CPFA_loop_functions::SetTrial(unsigned int v) {
}

void CPFA_loop_functions::setScore(double s) {
	score = s;
	if (score >= FoodItemCount) {
		PostExperiment();
	}
}

double CPFA_loop_functions::Score() {	
	return score;
}

void CPFA_loop_functions::ConfigureFromGenome(Real* g)
{
	// Assign genome generated by the GA to the appropriate internal variables.
	ProbabilityOfSwitchingToSearching = g[0];
	ProbabilityOfReturningToNest      = g[1];
	UninformedSearchVariation.SetValue(g[2]);
	RateOfInformedSearchDecay         = g[3];
	RateOfSiteFidelity                = g[4];
	RateOfLayingPheromone             = g[5];
	RateOfPheromoneDecay              = g[6];
}

REGISTER_LOOP_FUNCTIONS(CPFA_loop_functions, "CPFA_loop_functions")
