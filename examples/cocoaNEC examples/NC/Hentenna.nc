model( "Hentenna" ) 
{
	real base ;
	real feed ;
	real top ;
	real width ;
	real wavelength, freq ;

	freq= 28.100 ;
	wavelength = c/freq ;

	base = 6 ;
	feed = base + wavelength/8.3 ;
	top = base + wavelength/2.0 ;
	width = wavelength/6 ;

	voltageFeed( wire( 0, 0, feed, 0, width, feed, #10, 21 ), 1, 0 ) ;
	wire( 0, 0, feed, 0, 0, base, #10, 9 ) ;
	wire( 0, 0, base, 0, width, base, #10, 9 ) ;
	wire( 0, width, base, 0, width, feed, #10, 9 ) ;

	wire( 0, 0, feed, 0, 0, top, #10, 9 ) ;
	wire( 0, 0, top, 0, width, top, #10, 9 ) ;
	wire( 0, width, top, 0, width, feed, #10, 9 ) ;

	setFrequency( freq ) ;

}
