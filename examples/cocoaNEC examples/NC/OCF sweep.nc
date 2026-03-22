//  Demonstration of the keepDataBetweenModelRuns.
//  Plots multiple feedpiont impedances on the same Smith Chart.
//  An OCF dipole is fed at different locations, starting at the center and moving to 5% of the length.

float fraction ;

model ("OCF sweep" )
{
	float w, a, b ;
	element fed ;
	
	w = 5.165 ;
	a = ( w*2 )*fraction - w - 0.05 ;
	b = a + 0.1 ;
	
	//	three segment wire from -w to +w
	wire ( -w, 0, 0, a, 0, 0, #12, 21 ) ;
	fed = wire ( a, 0, 0, b, 0, 0, #14, 5 ) ;
	wire ( b, 0, 0, w, 0, 0, #12, 21 ) ;
	voltageFeed( fed, 1, 0 ) ;
	
	freespace() ;
}

control()
{
	keepDataBetweenModelRuns( 1 ) ;
	fraction = 0.5 ;
	repeat ( 45 ) {
		runModel() ;
		fraction = fraction - 0.01 ;
	}
}
