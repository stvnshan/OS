#include <types.h>
#include <lib.h>
#include <synchprobs.h>
#include <synch.h>
#include <opt-A1.h>
#include <array.h>

/* 
 * This simple default synchronization mechanism allows only vehicle at a time
 * into the intersection.   The intersectionSem is used as a a lock.
 * We use a semaphore rather than a lock so that this code will work even
 * before locks are implemented.
 */

/* 
 * Replace this default synchronization mechanism with your own (better) mechanism
 * needed for your solution.   Your mechanism may use any of the available synchronzation
 * primitives, e.g., semaphores, locks, condition variables.   You are also free to 
 * declare other global variables if your solution requires them.
 */

/*
 * replace this with declarations of any synchronization and other variables you need here
 */
//static struct semaphore *intersectionSem;

typedef struct cars{
  Direction origin;
  Direction destination;
} car;
static struct lock *intersection;  
// static struct cv *intersectionCv;
// volatile static struct array *carsAtIntersection;

static struct cv *cv_n;
static struct cv *cv_s;
static struct cv *cv_w;
static struct cv *cv_e;
static Direction dir;

static int carsWaiting[4];
static int carsMoving[4];



/* 
 * The simulation driver will call this function once before starting
 * the simulation
 *
 * You can use it to initialize synchronization and other variables.
 * 
 */
void
intersection_sync_init(void)
{
  /* replace this default implementation with your own implementation */

  //intersectionSem = sem_create("intersectionSem",1);

  intersection = lock_create("intersection");
  if (intersection == NULL) {
    panic("could not create intersection lock");
  }
  // intersectionCv = cv_create("intersectionCv");
  // if (intersectionCv == NULL) {
  //   panic("could not create intersection condition var");
  // }
  // carsAtIntersection = array_create();
  // if (carsAtIntersection == NULL) {
  //   panic("could not create array");
  // }
  cv_n = cv_create("cv_n");
  if (cv_n == NULL) {
    panic("could not create cv_n");
  }
  cv_s = cv_create("cv_s");
  if (cv_s == NULL) {
    panic("could not create cv_s");
  }
  cv_w = cv_create("cv_w");
  if (cv_w == NULL) {
    panic("could not create cv_w");
  }
  cv_e = cv_create("cv_e");
  if (cv_e == NULL) {
    panic("could not create cv_e");
  }
  dir = 5;
  for(int i = 0;i < 4;i++)  carsWaiting[i] = 0;
  for(int i = 0;i < 4;i++)  carsMoving[i] = 0;
  
  return;
}

/* 
 * The simulation driver will call this function once after
 * the simulation has finished
 *
 * You can use it to clean up any synchronization and other variables.
 *
 */
void
intersection_sync_cleanup(void)
{
  /* replace this default implementation with your own implementation */
  KASSERT(intersection != NULL);
  // KASSERT(intersectionCv != NULL);
  // KASSERT(carsAtIntersection != NULL);
  KASSERT(cv_n != NULL);
  KASSERT(cv_w != NULL);
  KASSERT(cv_e != NULL);
  KASSERT(cv_s != NULL);
  lock_destroy(intersection);

  // cv_destroy(intersectionCv);
  // array_destroy(carsAtIntersection);

  cv_destroy(cv_n);
  cv_destroy(cv_w);
  cv_destroy(cv_e);
  cv_destroy(cv_s);
}


/*
 * The simulation driver will call this function each time a vehicle
 * tries to enter the intersection, before it enters.
 * This function should cause the calling simulation thread 
 * to block until it is OK for the vehicle to enter the intersection.
 *
 * parameters:
 *    * origin: the Direction from which the vehicle is arriving
 *    * destination: the Direction in which the vehicle is trying to go
 *
 * return value: none
 */

bool if_goright(Direction origin, Direction destination){
  if(origin == north && destination == west){
    return true;
  }else if(origin == west && destination == south){
    return true;
  }else if(origin == south && destination == east){
    return true;
  }else if(origin == east && destination == north){
    return true;
  }
  return false;
}


// bool checkConstraints(Direction origin, Direction destination){
//   bool ifR = if_goright(origin,destination);
//   for(unsigned i = 0; i<array_num(carsAtIntersection);i++){
//     car *tmp = array_get(carsAtIntersection,i);
//     if(tmp->origin == origin && tmp->destination == destination) return false;
//     if(tmp->origin == origin) continue;
//     if(tmp->origin == destination && tmp->destination == origin) continue;
//     if(ifR && destination != tmp->destination) continue;
//     if(if_goright(tmp->origin,tmp->destination) && destination != tmp->destination) continue;
//     return false;
//   }
//   return true;
// }

void
intersection_before_entry(Direction origin, Direction destination) 
{
  /* replace this default implementation with your own implementation */
  (void)origin;  /* avoid compiler complaint about unused parameter */
  (void)destination; /* avoid compiler complaint about unused parameter */
  KASSERT(intersection != NULL);
  // KASSERT(intersectionCv != NULL);
  KASSERT(cv_n != NULL);
  KASSERT(cv_w != NULL);
  KASSERT(cv_e != NULL);
  KASSERT(cv_s != NULL);
  
  lock_acquire(intersection);
  
  // while(!checkConstraints(origin,destination)){
  //   cv_wait(intersectionCv,intersection);
  // }
  
  // car* c = kmalloc(sizeof(car));
  // c->destination = destination;
  // c->origin = origin;
  // array_add(carsAtIntersection, c ,NULL);
  if(dir == 5){
    dir = origin;
    carsMoving[dir]++;
  }else if(dir == origin){
    for(unsigned i = 0;i<4;i++){
      if(carsMoving[i] > 0 && i != origin){
        
        carsWaiting[origin]++;
        if(origin == north){
          cv_wait(cv_n,intersection);
        }else if(origin == south){
          cv_wait(cv_s,intersection);
        }else if(origin == west){
          cv_wait(cv_w,intersection);
        }else if(origin == east){
          cv_wait(cv_e,intersection);
        }
        while(dir != origin){
          if(origin == north){
            cv_wait(cv_n,intersection);
          }else if(origin == south){
            cv_wait(cv_s,intersection);
          }else if(origin == west){
            cv_wait(cv_w,intersection);
          }else if(origin == east){
            cv_wait(cv_e,intersection);
          }
        }
        carsWaiting[origin]--;
        break;
      }
    }
    
    carsMoving[dir]++;
  }else if(dir != origin){
    carsWaiting[origin]++;
    while(dir != origin){
      if(origin == north){
        cv_wait(cv_n,intersection);
      }else if(origin == south){
        cv_wait(cv_s,intersection);
      }else if(origin == west){
        cv_wait(cv_w,intersection);
      }else if(origin == east){
        cv_wait(cv_e,intersection);
      }
    }
    
    carsWaiting[origin]--;
    carsMoving[origin]++;
  }
  lock_release(intersection);
  return;
}


/*
 * The simulation driver will call this function each time a vehicle
 * leaves the intersection.
 *
 * parameters:
 *    * origin: the Direction from which the vehicle arrived
 *    * destination: the Direction in which the vehicle is going
 *
 * return value: none
 */

void
intersection_after_exit(Direction origin, Direction destination) 
{
  /* replace this default implementation with your own implementation */
  (void)origin;  /* avoid compiler complaint about unused parameter */
  (void)destination; /* avoid compiler complaint about unused parameter */
  // KASSERT(intersectionSem != NULL);
  // V(intersectionSem);
  KASSERT(intersection != NULL);
  // KASSERT(intersectionCv != NULL);
  // KASSERT(array_num(carsAtIntersection) != 0);

  KASSERT(cv_n != NULL);
  KASSERT(cv_w != NULL);
  KASSERT(cv_e != NULL);
  KASSERT(cv_s != NULL);
  lock_acquire(intersection);
  // unsigned index = -1;  
  // for(unsigned i = 0; i < array_num(carsAtIntersection);i++){
  //   car *tmp = array_get(carsAtIntersection,i);
  //   if(origin == tmp->origin && destination == tmp->destination){
  //     index = i;
  //     break;
  //   }
  // }
  // array_remove(carsAtIntersection,index);
  // cv_broadcast(intersectionCv,intersection);
  carsMoving[origin]--;
  
  if (carsMoving[origin] == 0){
	if(carsWaiting[0] == 0 && carsWaiting[1] == 0 && carsWaiting[2] == 0 && carsWaiting[3] == 0){
		dir =5;
	}else if(origin == north){
		if(carsWaiting[south] > 0){
			dir = south;
			cv_broadcast(cv_s,intersection);
		}else if(carsWaiting[west] > 0){
			dir = west;
			cv_broadcast(cv_w, intersection);
		}else if(carsWaiting[east]>0){
			dir = east;
			cv_broadcast(cv_e, intersection);
		}
	}else if(origin == south){
		if(carsWaiting[west] > 0 ){
			dir = west;
			cv_broadcast(cv_w, intersection);
		}else if(carsWaiting[east] > 0){
			dir = east;
			cv_broadcast(cv_e, intersection);
		}else if(carsWaiting[north] > 0){
			dir = north;
			cv_broadcast(cv_n, intersection);
		}
	}else if(origin == west){
		if(carsWaiting[east] > 0){
			dir = east;
			cv_broadcast(cv_e, intersection);
		}else if(carsWaiting[north] > 0){
			dir = north;
			cv_broadcast(cv_n, intersection);
		}else if(carsWaiting[south] > 0){
			dir = south;
			cv_broadcast(cv_s, intersection);
		}
	}else if(origin == east){
		if(carsWaiting[north] > 0){
			dir = north;
			cv_broadcast(cv_n, intersection);
		}else if(carsWaiting[south] > 0){
			dir = south;
			cv_broadcast(cv_s, intersection);
		}else if(carsWaiting[west] > 0){
			dir = west;
			cv_broadcast(cv_w, intersection);
		}
	}
  }
  
  lock_release(intersection);
  return;
}
