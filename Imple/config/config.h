// config.h
#ifndef CONFIG_H
#define CONFIG_H

//----- def for mster -----//
#define PROCESS_NUMBER 6;

//----- def for server -----//
#define DRONE_ICON 'X'
#define MAX_OBST_ARR_SIZE 20 // parameter also for for obstacle process
#define MAX_TARG_ARR_SIZE 5 // parameter also for the target process
// big circle for the repulsive force near the obstacle
#define OBST_RADIUS_FAR 3
// smlcircle for the repulsive force near the obstacle
#define OBST_RADIUS_CLOSE 1
// constant for the computation of the force
// constant for the nearest circe around the obstace
#define K_CLOSE 3
// constant for the repulsive force in the cirle lee near the obstace
#define K_STD 0.8

//----- def for drone -----//
#define INP_NUM 8
#define SSLOW 500
#define SLOW 300
#define MEDIUM 200
#define FAST 100
#define FFAST 50
#define CYCLE 1

//----- def for target -----//
#define TARGET_NUMBER 10
#define WIND_NUMBER 12
#define LIMIT 5


//----- def for obstacle -----//
// Seconds of upgrade of the obstacles
#define N 5
#define OBSTACLES_NUMBER 20

//----- def for socket_server -----//
#define SAI struct sockaddr_in
#define SA struct sockaddr
#define HE struct hostent
#define MAX_MSG_LENGHT 1024


#endif // CONFIG_H
