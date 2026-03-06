model( "vertical with raised radials" )
{
	real feed ;
	real bottom ;	

	feed = 0.5" ;
	bottom = 25" ;

	//  voltage feed a short feed section
	voltageFeed( wire( 0, 0, bottom, 0, 0, feed+bottom, #10, 21 ), 1.0, 0.0 ) ;

	//  the rest of the vertical
	wire( 0, 0, feed+bottom, 0, 0, 5.16, #10, 21 ) ;

	//  five raised radials
	radials( 0, 0, bottom, 5.8, #14, 5 ) ;

	//  use extended thin-wire approximation for greater accuracy since feed segment is short
	useExtendedKernel( 1 ) ;

	//  use good ground
	goodGround() ;
}