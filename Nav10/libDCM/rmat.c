// This file is part of MatrixPilot.
//
//    http://code.google.com/p/gentlenav/
//
// Copyright 2009, 2010 MatrixPilot Team
// See the AUTHORS.TXT file for a list of authors of MatrixPilot.
//
// MatrixPilot is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// MatrixPilot is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with MatrixPilot.  If not, see <http://www.gnu.org/licenses/>.


#include "libDCM_internal.h"
#include "..\libUDB\libUDB_internal.h"

//		These are the routines for maintaining a direction cosine matrix
//		that can be used to transform vectors between the earth and plane
//		coordinate systems. The 9 direction cosines in the matrix completely
//		define the orientation of the plane with respect to the earth.
//		The inverse of the matrix is equal to its transpose. This defines
//		the so-called orthogonality conditions, which impose 6 constraints on
//		the 9 elements of the matrix.

//	All numbers are stored in 2.14 format.
//	Vector and matrix libraries work in 1.15 format.
//	This combination allows values of matrix elements between -2 and +2.
//	Multiplication produces results scaled by 1/2.


#define RMAX15 0b0110000000000000	//	1.5 in 2.14 format

#define GGAIN SCALEGYRO*6*(RMAX*0.025)		//	integration multiplier for gyros 15mv/degree/sec
fractional ggain = GGAIN ;

#if ( BOARD_TYPE == UDB3_BOARD )
//Paul's gains corrected for GGAIN
#define KPROLLPITCH 256*5
#define KIROLLPITCH 256
#else
//Paul's gains:
#define KPROLLPITCH 256*10
#define KIROLLPITCH 256*2
#endif

#define KPYAW 256*4
#define KIYAW 32

#define GYROSAT 15000
// threshold at which gyros may be saturated

//	rmat is the matrix of direction cosines relating
//	the body and earth coordinate systems.
//	The columns of rmat are the axis vectors of the plane,
//	as measured in the earth reference frame.
//	rmat is initialized to the identity matrix in 2.14 fractional format
fractional IMPORTANT rmat[] = { RMAX , 0 , 0 , 0 , RMAX , 0 , 0 , 0 , RMAX } ;

//	rup is the rotational update matrix.
//	At each time step, the new rmat is equal to the old one, multiplied by rup.
fractional IMPORTANT rup[] = { RMAX , 0 , 0 , 0 , RMAX , 0 , 0 , 0 , RMAX } ;

//	gyro rotation vector:
fractional IMPORTANT omegagyro[] = { 0 , 0 , 0 } ;
fractional IMPORTANT omega[] = { 0 , 0 , 0 } ;

//	gyro correction vectors:
fractional IMPORTANT omegacorrP[] = { 0 , 0 , 0 } ;
fractional IMPORTANT omegacorrI[] = { 0 , 0 , 0 } ;

//  acceleration, as measured in GPS earth coordinate system
fractional IMPORTANT accelEarth[] = { 0 , 0 , 0 } ;

union longww IMPORTANT accelEarthFiltered[] = { { 0 } , { 0 } ,  { 0 } } ;

//	correction vector integrators ;
union longww IMPORTANT gyroCorrectionIntegral[] =  { { 0 } , { 0 } ,  { 0 } } ;

//	accumulator for computing adjusted omega:
fractional IMPORTANT omegaAccum[] = { 0 , 0 , 0 } ;

//	gravity, as measured in plane coordinate system
fractional IMPORTANT gplane[] = { 0 , 0 , GRAVITY } ;

//	horizontal velocity over ground, as measured by GPS (Vz = 0 )
fractional IMPORTANT dirovergndHGPS[] = { 0 , RMAX/2 , 0 } ;

//	horizontal direction over ground, as indicated by Rmatrix
fractional IMPORTANT dirovergndHRmat[] = { 0 , RMAX/2 , 0 } ;

//	rotation angle equal to omega times integration factor:
fractional IMPORTANT theta[] = { 0 , 0 , 0 } ;

//	matrix buffer:
fractional IMPORTANT rbuff[] = { 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 } ;

//	vector buffer
fractional IMPORTANT errorRP[] = { 0 , 0 , 0 } ;
fractional IMPORTANT errorYawground[] = { 0 , 0 , 0 } ;
fractional IMPORTANT errorYawplane[]  = { 0 , 0 , 0 } ;

//	measure of error in orthogonality, used for debugging purposes:
fractional IMPORTANTz error;

fractional IMPORTANTz declinationVector[2];

signed char DECLINATIONANGLE PARAMETER = ((signed char)(MAGNETICDECLINATION*128/180));

void dcm_init_rmat( void )
{
#if ( MAG_YAW_DRIFT == 1 )
	declinationVector[0] = cosine(DECLINATIONANGLE) ;
	declinationVector[1] = sine(DECLINATIONANGLE) ;
#endif
}


//	Implement the cross product. *dest = *src1X*src2 ;
void VectorCross( fractional * dest , fractional * src1 , fractional * src2 )
{
	union longww crossaccum ;
	crossaccum.WW = __builtin_mulss( src1[1] , src2[2] ) ;
	crossaccum.WW -= __builtin_mulss( src1[2] , src2[1] ) ;
	crossaccum.WW *= 4 ;
	dest[0] = crossaccum._.W1 ;
	crossaccum.WW = __builtin_mulss( src1[2] , src2[0] ) ;
	crossaccum.WW -= __builtin_mulss( src1[0] , src2[2] ) ;
	crossaccum.WW *= 4 ;
	dest[1] = crossaccum._.W1 ;
	crossaccum.WW = __builtin_mulss( src1[0] , src2[1] ) ;
	crossaccum.WW -= __builtin_mulss( src1[1] , src2[0] ) ;
	crossaccum.WW *= 4 ;
	dest[2] = crossaccum._.W1 ;
	return ;
}

int vref_adj IMPORTANT = 0;

void read_gyros()
//	fetch the gyro signals and subtract the baseline offset, 
//	and adjust for variations in supply voltage
{
//	int gx , gy , gz ;
#ifdef VREF
	vref_adj = (udb_vref.offset>>1) - (udb_vref.value>>1) ;
#else
	vref_adj = 0 ;
#endif

#if ( HILSIM == 1 )
	omegagyro[0] = q_sim.BB;
	omegagyro[1] = p_sim.BB;
	omegagyro[2] = r_sim.BB;  
#else
	omegagyro[0] = XRATE_VALUE ;
	omegagyro[1] = YRATE_VALUE ;
	omegagyro[2] = ZRATE_VALUE ;
#endif
	return ;
}

void read_accel()
{
#if ( HILSIM == 1 )
	gplane[0] = v_dot_sim.BB;
	gplane[1] = u_dot_sim.BB; 
	gplane[2] = w_dot_sim.BB;
#else
	gplane[0] =   XACCEL_VALUE ;
	gplane[1] =   YACCEL_VALUE ;
	gplane[2] =   ZACCEL_VALUE ;
#endif
	udb_setDSPLibInUse(true) ;
	
	accelEarth[0] =  VectorDotProduct( 3 , &rmat[0] , gplane )<<1;
	accelEarth[1] = - VectorDotProduct( 3 , &rmat[3] , gplane )<<1;
	accelEarth[2] = -((int)GRAVITY) + (VectorDotProduct( 3 , &rmat[6] , gplane )<<1);

	accelEarthFiltered[0].WW += ((((long)accelEarth[0])<<16) - accelEarthFiltered[0].WW)>>5 ;
	accelEarthFiltered[1].WW += ((((long)accelEarth[1])<<16) - accelEarthFiltered[1].WW)>>5 ;
	accelEarthFiltered[2].WW += ((((long)accelEarth[2])<<16) - accelEarthFiltered[2].WW)>>5 ;
	
	udb_setDSPLibInUse(false) ;
	return ;
}

//	multiplies omega times speed, and scales appropriately
int omegaSOG ( int omega , unsigned int speed  )
{
	union longww working ;
	speed = speed>>3 ;
	working.WW = __builtin_mulsu( omega , speed ) ;
	if ( ((int)working._.W1 )> ((int)CENTRIFSAT) )
	{
		return RMAX ;
	}
	else if ( ((int)working._.W1) < ((int)-CENTRIFSAT) )
	{
		return - RMAX ;
	}
	else
	{
		working.WW = working.WW>>5 ;
		working.WW = __builtin_mulsu( working._.W0 , CENTRISCALE ) ;
		working.WW = working.WW<<5 ;
		return working._.W1 ;
	}
}

void adj_accel()
{
	gplane[0]=gplane[0]- omegaSOG( omegaAccum[2] , (unsigned int) sog_gps.BB ) ;
	gplane[2]=gplane[2]+ omegaSOG( omegaAccum[0] , (unsigned int) sog_gps.BB ) ;
	gplane[1]=gplane[1]+ ((int)(ACCELSCALE))*forward_acceleration ;
	
	return ;
}

//	The update algorithm!!
void rupdate(void)
//	This is the key routine. It performs a small rotation
//	on the direction cosine matrix, based on the gyro vector and correction.
//	It uses vector and matrix routines furnished by Microchip.
{
	udb_setDSPLibInUse(true) ;
	VectorAdd( 3 , omegaAccum , omegagyro , omegacorrI ) ;
	VectorAdd( 3 , omega , omegaAccum , omegacorrP ) ;
	//	scale by the integration factor:
	VectorScale( 3 , theta , omega , ggain ) ;	// Scalegain of 2
	//	construct the off-diagonal elements of the update matrix:
	rup[1] = -theta[2] ;
	rup[2] =  theta[1] ;
	rup[3] =  theta[2] ;
	rup[5] = -theta[0] ;
	rup[6] = -theta[1] ;
	rup[7] =  theta[0] ;
	//	matrix multiply the rmatrix by the update matrix
	MatrixMultiply( 3 , 3 , 3 , rbuff , rmat , rup ) ;
	//	multiply by 2 and copy back from rbuff to rmat:
	MatrixAdd( 3 , 3 , rmat , rbuff , rbuff ) ; 
	udb_setDSPLibInUse(false) ;
	return ;
}

//	normalization algorithm:
void normalize(void)
//	This is the routine that maintains the orthogonality of the
//	direction cosine matrix, which is expressed by the identity
//	relationship that the cosine matrix multiplied by its
//	transpose should equal the identity matrix.
//	Small adjustments are made at each time step to assure orthogonality.
{
	fractional norm ; // actual magnitude
	fractional renorm ;	// renormalization factor
	udb_setDSPLibInUse(true) ;
	//	compute -1/2 of the dot product between rows 1 and 2
	error =  - VectorDotProduct( 3 , &rmat[0] , &rmat[3] ) ; // note, 1/2 is built into 2.14
	//	scale rows 1 and 2 by the error
	VectorScale( 3 , &rbuff[0] , &rmat[3] , error ) ;
	VectorScale( 3 , &rbuff[3] , &rmat[0] , error ) ;
	//	update the first 2 rows to make them closer to orthogonal:
	VectorAdd( 3 , &rbuff[0] , &rbuff[0] , &rmat[0] ) ;
	VectorAdd( 3 , &rbuff[3] , &rbuff[3] , &rmat[3] ) ;
	//	use the cross product of the first 2 rows to get the 3rd row
	VectorCross( &rbuff[6] , &rbuff[0] , &rbuff[3] ) ;


	//	Use a Taylor's expansion for 1/sqrt(X*X) to avoid division in the renormalization
	//	rescale row1
	norm = VectorPower( 3 , &rbuff[0] ) ; // Scalegain of 0.5
	renorm = RMAX15 - norm ;
	VectorScale( 3 , &rbuff[0] , &rbuff[0] , renorm ) ;
	VectorAdd( 3 , &rmat[0] , &rbuff[0] , &rbuff[0] ) ;
	//	rescale row2
	norm = VectorPower( 3 , &rbuff[3] ) ;
	renorm = RMAX15 - norm ;
	VectorScale( 3 , &rbuff[3] , &rbuff[3] , renorm ) ;
	VectorAdd( 3 , &rmat[3] , &rbuff[3] , &rbuff[3] ) ;
	//	rescale row3
	norm = VectorPower( 3 , &rbuff[6] ) ;
	renorm = RMAX15 - norm ;
	VectorScale( 3 , &rbuff[6] , &rbuff[6] , renorm ) ;
	VectorAdd( 3 , &rmat[6] , &rbuff[6] , &rbuff[6] ) ;
	udb_setDSPLibInUse(false) ;
	return ;
}

void roll_pitch_drift()
{
	udb_setDSPLibInUse(true) ;
	VectorCross( errorRP , gplane , &rmat[6] ) ;
	udb_setDSPLibInUse(false) ;
	return ;
}

void yaw_drift()
{
	//	although yaw correction is done in horizontal plane,
	//	this is done in 3 dimensions, just in case we change our minds later
	//	form the horizontal direction over ground based on rmat
	if (dcm_flags._.yaw_req )
	{
		if ( velocity_magnitude > GPS_SPEED_MIN )
		{
			udb_setDSPLibInUse(true) ;
			//	vector cross product to get the rotation error in ground frame
			VectorCross( errorYawground , dirovergndHRmat , dirovergndHGPS ) ;
			//	convert to plane frame:
			//	*** Note: this accomplishes multiplication rmat transpose times errorYawground!!
			MatrixMultiply( 1 , 3 , 3 , errorYawplane , errorYawground , rmat ) ;
			udb_setDSPLibInUse(false) ;
		}
		else
		{
			errorYawplane[0] = errorYawplane[1] = errorYawplane[2] = 0 ;
		}
		
		dcm_flags._.yaw_req = 0 ;
	}
	return ;
}

#if (MAG_YAW_DRIFT == 1)

fractional magFieldEarth[3] ;

extern fractional udb_magFieldBody[3] ;
extern fractional udb_magOffset[3] ;

fractional magFieldEarthPrevious[3] ;
fractional magFieldBodyPrevious[3] ;

fractional rmatPrevious[9] ;

int offsetDelta[3] ;

void align_rmat_to_mag(void)
{
	unsigned char theta ;
	struct relative2D initialBodyField ;
	int costheta ;
	int sintheta ;
	initialBodyField.x = udb_magFieldBody[0] ;
	initialBodyField.y = udb_magFieldBody[1] ;
#if (BOARD_TYPE == ASPG_BOARD)
//	theta = rect_to_polar( &initialBodyField ) -128 - DECLINATIONANGLE ; // faces south
//	theta = rect_to_polar( &initialBodyField ) +64 - DECLINATIONANGLE ; // faces east
	theta = rect_to_polar( &initialBodyField ) -64 - DECLINATIONANGLE ;
#else
	theta = rect_to_polar( &initialBodyField ) -64 - DECLINATIONANGLE ;
#endif
	costheta = cosine(theta) ;
	sintheta = sine(theta) ;
	rmat[0] = rmat[4] = costheta ;
	rmat[1] = sintheta ;
	rmat[3] = - sintheta ;
	return ;
}

void mag_drift()
{
	int mag_error ;
	int vector_index ;
	fractional rmatTransposeMagField[3] ;
	fractional offsetSum[3] ;
	
	if ( dcm_flags._.mag_drift_req )
	{
		udb_setDSPLibInUse(true) ;

		if ( dcm_flags._.first_mag_reading == 1 )
		{
			align_rmat_to_mag() ;
		}

		magFieldEarth[0] = VectorDotProduct( 3 , &rmat[0] , udb_magFieldBody )<<1 ;
		magFieldEarth[1] = VectorDotProduct( 3 , &rmat[3] , udb_magFieldBody )<<1 ;
		magFieldEarth[2] = VectorDotProduct( 3 , &rmat[6] , udb_magFieldBody )<<1 ;

		mag_error = 100*VectorDotProduct( 2 , magFieldEarth , declinationVector ) ; // Dotgain = 1/2
		VectorScale( 3 , errorYawplane , &rmat[6] , mag_error ) ; // Scalegain = 1/2

		VectorAdd( 3 , offsetSum , udb_magFieldBody , magFieldBodyPrevious ) ;
		for ( vector_index = 0 ; vector_index < 3 ; vector_index++ )
		{
			offsetSum[vector_index] >>= 1 ;
		}

		MatrixMultiply( 1 , 3 , 3 , rmatTransposeMagField , magFieldEarthPrevious , rmat ) ;
		VectorSubtract( 3 , offsetSum , offsetSum , rmatTransposeMagField ) ;

		MatrixMultiply( 1 , 3 , 3 , rmatTransposeMagField , magFieldEarth , rmatPrevious ) ;
		VectorSubtract( 3 , offsetSum , offsetSum , rmatTransposeMagField ) ;

		for ( vector_index = 0 ; vector_index < 3 ; vector_index++ )
		{
			int adjustment ;
			adjustment = offsetSum[vector_index] ;
			if ( abs( adjustment ) < 3 )
			{
				offsetSum[vector_index] = 0 ;
				adjustment = 0 ;
			}
			offsetDelta[vector_index] = adjustment ;
		}

		if ( dcm_flags._.first_mag_reading == 0 )
		{
			VectorAdd ( 3 , udb_magOffset , udb_magOffset , offsetSum ) ;
		}
		else
		{
			dcm_flags._.first_mag_reading = 0 ;
		}

		VectorCopy ( 3 , magFieldEarthPrevious , magFieldEarth ) ;
		VectorCopy ( 3 , magFieldBodyPrevious , udb_magFieldBody ) ;
		VectorCopy ( 9 , rmatPrevious , rmat ) ;

		udb_setDSPLibInUse(false) ;
		dcm_flags._.mag_drift_req = 0 ;
	}
	return ;
}

#endif


void PI_feedback(void)
{
	fractional errorRPScaled[3] ;
	
	udb_setDSPLibInUse(true) ;
	
	VectorScale( 3 , omegacorrP , errorYawplane , KPYAW ) ; // Scale gain = 2
	VectorScale( 3 , errorRPScaled , errorRP , KPROLLPITCH ) ; // Scale gain = 2
	VectorAdd( 3 , omegacorrP , omegacorrP , errorRPScaled ) ;
	
	udb_setDSPLibInUse(false) ;
	
	gyroCorrectionIntegral[0].WW += ( __builtin_mulss( errorRP[0] , KIROLLPITCH )>>3) ;
	gyroCorrectionIntegral[1].WW += ( __builtin_mulss( errorRP[1] , KIROLLPITCH )>>3) ;
	gyroCorrectionIntegral[2].WW += ( __builtin_mulss( errorRP[2] , KIROLLPITCH )>>3) ;

	gyroCorrectionIntegral[0].WW += ( __builtin_mulss( errorYawplane[0] , KIYAW )>>3) ;
	gyroCorrectionIntegral[1].WW += ( __builtin_mulss( errorYawplane[1] , KIYAW )>>3) ;
	gyroCorrectionIntegral[2].WW += ( __builtin_mulss( errorYawplane[2] , KIYAW )>>3) ;

	omegacorrI[0] = gyroCorrectionIntegral[0]._.W1>>3 ;
	omegacorrI[1] = gyroCorrectionIntegral[1]._.W1>>3 ;
	omegacorrI[2] = gyroCorrectionIntegral[2]._.W1>>3 ;

	return ;
}

/*
void output_matrix(void)
//	This routine makes the direction cosine matrix evident
//	by setting the three servos to the three values in the
//	matrix.
{
	union longww accum ;
	accum.WW = __builtin_mulss( rmat[6] , 4000 ) ;
//	PDC1 = 3000 + accum._.W1 ;
//	accum.WW = __builtin_mulss( rmat[7] , 4000 ) ;
	accum.WW = __builtin_mulss( rmat[3] , 4000 ) ;
	PDC2 = 3000 + accum._.W1 ;
	accum.WW = __builtin_mulss( rmat[4] , 4000 ) ;
	PDC3 = 3000 + accum._.W1 ;
	return ;
}
*/

/*
void output_IMUvelocity(void)
{
	PDC1 = pulsesat( IMUvelocityx._.W1 + 3000 ) ;
	PDC2 = pulsesat( IMUvelocityy._.W1 + 3000 ) ;
	PDC3 = pulsesat( IMUvelocityz._.W1 + 3000 ) ;

//	PDC1 = pulsesat( accelEarth[0] + 3000 ) ;
//	PDC2 = pulsesat( accelEarth[1] + 3000 ) ;
//	PDC3 = pulsesat( accelEarth[2] + 3000 ) ;

	return ;
}
*/

extern void dead_reckon(void) ;
unsigned int lastGyroSamples, lastAccelSamples;

void dcm_run_imu_step(void)
//	Read the gyros and accelerometers, 
//	update the matrix, renormalize it, 
//	adjust for roll and pitch drift,
//	and send it to the servos.
{

	read_gyros() ;

	interrupt_save_extended_state;
	read_accel() ;
	dead_reckon() ;
#if ( HILSIM != 1 )
	adj_accel() ;
#endif
	rupdate() ;
	normalize() ;
	roll_pitch_drift() ;
#if (MAG_YAW_DRIFT == 1)
	if ( CD[magCDindex].iResult >= MAG_NORMAL  )
		mag_drift() ;
	else yaw_drift() ;
#else
	yaw_drift() ;
#endif
	PI_feedback() ;
	
	interrupt_restore_extended_state ;
	return ;
}

