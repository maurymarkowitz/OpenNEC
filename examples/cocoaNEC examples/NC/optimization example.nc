//  Optimization example
//  Optimization loop is run on the length of the dipole to minimize the VSWR
//  A pause is place in the loop so the animation in the Output window is slowed down
//  A table of SVSWR values vs dipole length is sent to the Output Console.

real y ;

model( "optimize dipole" ) 
{
	real height ;

	height = 12 ;
	voltageFeed( wire( 0, -y, height, 0, y, height, #14, 21 ), 1.0, 0.0 ) ;
}

control()
{
	real dy, current, previous ;

	dy = 0.5 ;
	y = 4.0 ;
	previous = 999.9 ;

	repeat ( 30 ) {
		runModel() ;
		current = vswr( 1 ) ;
		printf( "y = %.2f  vswr = %.2f\n", y, current ) ;
		if ( current > previous ) dy = -dy*0.5 ;
		previous = current ;
		y = y + dy ;
		pause( 0.3 ) ;
	}
	printf( "--- done ---\n" ) ;
}
