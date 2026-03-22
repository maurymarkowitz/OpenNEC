//	http://www.naic.edu/~angel/kp4ao/ham/owa.html

model( "OWC yagi" ) 
{
	real x, y, z, r, p ;

	z = 20 ;
	r = 0.25" ;
	p =  48" + 24" + 44" + 36" ;
	setFrequency( 14.0 ) ;
	addFrequency( 14.1 ) ;
	addFrequency( 14.2 ) ;

	x = 0 ;
	y = p + 65.73" ;

	wire( x, -48", z, x, 48", z, 0.5", 21 ) ;
	wire( x, 48", z, x, 72", z, 0.4375", 11 ) ;
	wire( x, 72", z, x, 116", z, 0.375", 11 ) ;
	wire( x, 116", z, x, p, z, 0.3125", 11 ) ;
	wire( x, p, z, x, y, z, r, 11 ) ;
	wire( x, -48", z, x, -72", z, 0.4375", 11 ) ;
	wire( x, -72", z, x, -116", z, 0.375", 11 ) ;
	wire( x, -116", z, x, -p, z, 0.3125", 11 ) ;
	wire( x, -p, z, x, -y, z, r, 11 ) ;

	x = 90.0" ;
	y = p + 58.7" ;

	voltageFeed( wire( x, -48", z, x, 48", z, 0.5", 21 ), 1, 0 ) ;
	wire( x, 48", z, x, 72", z, 0.4375", 11 ) ;
	wire( x, 72", z, x, 116", z, 0.375", 11 ) ;
	wire( x, 116", z, x, p, z, 0.3125", 11 ) ;
	wire( x, p, z, x, y, z, r, 11 ) ;
	wire( x, -48", z, x, -72", z, 0.4375", 11 ) ;
	wire( x, -72", z, x, -116", z, 0.375", 11 ) ;
	wire( x, -116", z, x, -p, z, 0.3125", 11 ) ;
	wire( x, -p, z, x, -y, z, r, 11 ) ;

	x = 139.52" ;
	y = p + 48.8" ;

	wire( x, -48", z, x, 48", z, 0.5", 21 ) ;
	wire( x, 48", z, x, 72", z, 0.4375", 11 ) ;
	wire( x, 72", z, x, 116", z, 0.375", 11 ) ;
	wire( x, 116", z, x, p, z, 0.3125", 11 ) ;
	wire( x, p, z, x, y, z, r, 11 ) ;
	wire( x, -48", z, x, -72", z, 0.4375", 11 ) ;
	wire( x, -72", z, x, -116", z, 0.375", 11 ) ;
	wire( x, -116", z, x, -p, z, 0.3125", 11 ) ;
	wire( x, -p, z, x, -y, z, r, 11 ) ;

	x = 226.70" ;
	y = p + 42.62" ;

	wire( x, -48", z, x, 48", z, 0.5", 21 ) ;
	wire( x, 48", z, x, 72", z, 0.4375", 11 ) ;
	wire( x, 72", z, x, 116", z, 0.375", 11 ) ;
	wire( x, 116", z, x, p, z, 0.3125", 11 ) ;
	wire( x, p, z, x, y, z, r, 11 ) ;
	wire( x, -48", z, x, -72", z, 0.4375", 11 ) ;
	wire( x, -72", z, x, -116", z, 0.375", 11 ) ;
	wire( x, -116", z, x, -p, z, 0.3125", 11 ) ;
	wire( x, -p, z, x, -y, z, r, 11 ) ;

	x = 388.44" ;
	y = p + 42.63" ;

	wire( x, -48", z, x, 48", z, 0.5", 21 ) ;
	wire( x, 48", z, x, 72", z, 0.4375", 11 ) ;
	wire( x, 72", z, x, 116", z, 0.375", 11 ) ;
	wire( x, 116", z, x, p, z, 0.3125", 11 ) ;
	wire( x, p, z, x, y, z, r, 11 ) ;
	wire( x, -48", z, x, -72", z, 0.4375", 11 ) ;
	wire( x, -72", z, x, -116", z, 0.375", 11 ) ;
	wire( x, -116", z, x, -p, z, 0.3125", 11 ) ;
	wire( x, -p, z, x, -y, z, r, 11 ) ;

	x = 570.00" ;
	y = p + 35.39" ;

	wire( x, -48", z, x, 48", z, 0.5", 21 ) ;
	wire( x, 48", z, x, 72", z, 0.4375", 11 ) ;
	wire( x, 72", z, x, 116", z, 0.375", 11 ) ;
	wire( x, 116", z, x, p, z, 0.3125", 11 ) ;
	wire( x, p, z, x, y, z, r, 11 ) ;
	wire( x, -48", z, x, -72", z, 0.4375", 11 ) ;
	wire( x, -72", z, x, -116", z, 0.375", 11 ) ;
	wire( x, -116", z, x, -p, z, 0.3125", 11 ) ;
	wire( x, -p, z, x, -y, z, r, 11 ) ;

	freespace() ;

}
