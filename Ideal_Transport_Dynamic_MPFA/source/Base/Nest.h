#ifndef NEST_H_
#define NEST_H_

#include <map> //qilu 09/11/2016
#include <argos3/core/utility/math/vector2.h>
//#include <argos3/core/utility/math/ray3.h>
#include <argos3/core/utility/logging/argos_log.h>
#include "Pheromone.h"
using namespace argos;
using namespace std;

/*****
 * Implementation of the iAnt nest object used by the iAnt MPFA. iAnts
 * build and maintain a list of these nest objects.
 *****/
class Nest {

	public:
		      Nest();
		      Nest(CVector2 location);
		       
		      vector<Pheromone> PheromoneList;
        map<string, argos::CVector2> FidelityList; //qilu 09/10/2016
        map<string, size_t> DensityOnFidelity; //qilu 09/11/2016
        vector<CVector2> FoodList;
        vector<int> FoodDeliveryStartTime;
        vector<int> RobotReturnStartTime;
        size_t num_collected_tags;
                
        argos::Real NestRadius;
        argos::Real NestRadiusSquared; //qilu 12/2018
        // Dynamic Depot helper functions
        CVector2 NewLocation;
        size_t visited_time_point_in_minute;
		
        /* constructor function */
		
		      /* public helper functions */
        void      SetNestRadius(int level, Real radius);
        argos::Real GetNestRadius();
        argos::Real GetNestRadiusSquared();
        
        void  SetNestIdx(size_t idx);
        void SetParentNestIdx_with_backtrack(size_t idx, size_t NumOfBranch, map<int, Nest> *Nests);
        void SetParentNestIdx_no_backtrack(vector<Nest *> parents);
        void SetLevel(size_t idx);
        void SetRegionFlag(int flag);
        void SetDeliveryCapacity(size_t c);
        void SetDeliveryRobot(size_t n);
        size_t GetDeliveryRobot();
        void SetDeliveringTime(argos::Real t);
        argos::Real GetDeliveringTime();
        size_t GetDeliveryCapacity();
        size_t  GetNestIdx();
        size_t  GetParentNestIdx();
        size_t GetLevel();
        int GetRegionFlag();
        //size_t GetCollectedTagNum();


        // Dynamic Depot helper functions
        CVector2	GetLocation();
        void		SetLocation();
        void		SetLocation(CVector2 newLocation); //qilu 09/11/2016
        void     UpdateNestLocation(); //qilu 09/10/2016

	private:
        CVector2 nestLocation;
        size_t  nest_idx;
        size_t parent_nest_idx;
        size_t level;
        size_t capacity;
        size_t numDeliveryRobot;
        argos::Real DeliveringTime;
        
};

#endif /* IANT_NEST_H_ */
