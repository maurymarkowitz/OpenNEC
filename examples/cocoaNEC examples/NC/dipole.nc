model ( "dipole" )
{
	real height, length ;
	element driven ;

	height = 40' ;
	length = 5.0 ;
	driven = wire( 0, -length, height, 0, length, height, #14, 21 ) ;
	voltageFeed( driven, 1.0, 0.0 ) ;
}

