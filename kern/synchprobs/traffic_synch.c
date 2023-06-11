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
static struct cv *intersectionCv;
volatile static struct array *carsAtIntersection;


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
    panic("could not create intersection semaphore");
  }
  intersectionCv = cv_create("intersectionCv");
  if (intersectionCv == NULL) {
    panic("could not create intersection condition var");
  }
  carsAtIntersection = array_create();
  if (carsAtIntersection == NULL) {
    panic("could not create array");
  }
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
  KASSERT(intersectionCv != NULL);
  KASSERT(carsAtIntersection != NULL);
  lock_destroy(intersection);
  cv_destroy(intersectionCv);
  array_destroy(carsAtIntersection);
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


bool checkConstraints(Direction origin, Direction destination){
  bool ifR = if_goright(origin,destination);
  for(unsigned i = 0; i<array_num(carsAtIntersection);i++){
    car *tmp = array_get(carsAtIntersection,i);
    if(tmp->origin == origin && tmp->destination == destination) return false;
    if(tmp->origin == origin) continue;
    if(tmp->origin == destination && tmp->destination == origin) continue;
    if(ifR && destination != tmp->destination) continue;
    if(if_goright(tmp->origin,tmp->destination) && destination != tmp->destination) continue;
    return false;
  }
  return true;
}
void
intersection_before_entry(Direction origin, Direction destination) 
{
  /* replace this default implementation with your own implementation */
  (void)origin;  /* avoid compiler complaint about unused parameter */
  (void)destination; /* avoid compiler complaint about unused parameter */
  KASSERT(intersection != NULL);
  KASSERT(intersectionCv != NULL);
  
  lock_acquire(intersection);
  
  while(!checkConstraints(origin,destination)){
    cv_wait(intersectionCv,intersection);
  }
  
  car* c = kmalloc(sizeof(car));
  c->destination = destination;
  c->origin = origin;
  array_add(carsAtIntersection, c ,NULL);
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
  KASSERT(intersectionCv != NULL);
  KASSERT(array_num(carsAtIntersection) != 0);
  lock_acquire(intersection);
  unsigned index = -1;  
  for(unsigned i = 0; i < array_num(carsAtIntersection);i++){
    car *tmp = array_get(carsAtIntersection,i);
    if(origin == tmp->origin && destination == tmp->destination){
      index = i;
      break;
    }
  }
  array_remove(carsAtIntersection,index);
  cv_broadcast(intersectionCv,intersection);
  lock_release(intersection);
  return;
}
