//  Vertical antenna over salt water

model ( "ground mounted vertical" ) 
{
	element feedSegment ;
	real feedElementHeight, verticalHeight  ;

	//  basic geometries of the vertical antenna
	feedElementHeight = 2" ;
	verticalHeight = 5.18 ;

	//  short segment at the base of the vertical were we feed the antenna
	feedSegment = wire( 0, 0, 0, 0,  0, feedElementHeight, 0.1", 3 ) ;
	voltageFeed( feedSegment, 1, 0 ) ;

	//  the rest of the vertical
	wire( 0, 0, feedElementHeight, 0, 0, verticalHeight, 0.1", 21 ) ;

	//  turn on the NEC-2 ground radials; we need this to feed the vertical against
	//  12 radials 1/4 wavelength long, with #14 AWG wire
	necRadials( 5.18, #14, 12 ) ;
	
	//  model antenna over salt water
	saltWaterGround() ;
}