//  Waller Flag (compact low-band receiving beam)

model( "N4IS Waller" )
{
	element feed1, feed2, load1, load2 ;
	real base, height, width, x ;

	base = 40.0 ;
	height = 4.5 ;
	width = 2.1 ;

	// main loop
	x = 0 ;
	feed1 = wire( x, 0, base, x, 0, base+height, #12, 21 ) ;
	load1 = wire( x-width, 0, base, x-width, 0, base+height, #12, 21 ) ;
	wire( x, 0, base, x-width, 0, base, #12, 21 ) ;
	wire( x, 0, base+height, x-width, 0, base+height, #12, 21 ) ;

	//  second loop
	x = 5.7 ;
	feed2 = wire( x, 0, base, x, 0, base+height, #12, 21 ) ;
	load2 = wire( x-width, 0, base, x-width, 0, base+height, #12, 21 ) ;
	wire( x, 0, base, x-width, 0, base, #12, 21 ) ;
	wire( x, 0, base+height, x-width, 0, base+height, #12, 21 ) ;

	impedanceLoad( load1, 580, 0.0 ) ;
	impedanceLoad( load2, 600.0, 0.0 ) ;

	voltageFeed( feed1, 1.0, 0.0 ) ;
	voltageFeed( feed2, -0.95, -0.15 ) ;

	setFrequency( 1.900 ) ;
	goodGround() ;
}
