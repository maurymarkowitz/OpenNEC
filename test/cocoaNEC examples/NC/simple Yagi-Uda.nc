//  simple 2 element Yagi-Uda beam antenna over average ground

model( "simple Yagi-Uda" )
{
	real height, length, separation, reflector ;
	element driven ;

	height = 40' ;
	length = 5.18 ;
	separation = 2.0 ;
	reflector = 1.04 ;

	//  driven element
	driven = wire( 0, -length, height, 0, length, height, 1", 21 ) ;
	voltageFeed( driven, 1, 0 ) ;
	
	//  reflector, placed behind (negative x axis) the driven element
	wire( -separation, -length*reflector, height, -separation, length*reflector, height, 1", 21 ) ;

	//  use average ground
	averageGround() ;
}