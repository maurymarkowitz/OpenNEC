
//
// LPDA.nc
// Doug Hall K4DSP
// June 4, 2009
//
// LPDA.nc models a Log Periodic Dipole Array. Set the variables below for the number
// of elements, the boom length, the min and max frequencies, and the height. The program
// uses a recursive function to create the elements of the antenna and connects them with
// a crossed transmission line.
//
// This is a simplistic model. The usual LPDA calculation of Tau, Sigma, and Alpha is not
// performed. Instead, the elements are equally spaced on the boom, and the difference in
// length from one element to the next is a constant.

real boomdelta, eledelta;

model( "LPDA" )
{

	int numElements;
	real boom, lowestfreq, highestfreq;
	real longelement, shortelement;
	real wirex, wirey;
	real height;
	element e1;
	
	numElements = 8;
	boom = 18';
	lowestfreq = 13.0;		// MHz
	highestfreq = 30.0;	// MHz
	height = 50';
	
	// calculate the amount to shorten each successive element
	eledelta = ((492/lowestfreq) - (492/highestfreq)) / (numElements-1);
	eledelta = eledelta * 0.305;
	eledelta = eledelta / 2.0;

	// Calculate space between each element. Elements are equally spaced on the boom.
	boomdelta = boom / (numElements-1);
	
	// calculate longest and shortest elements
	longelement = (492/lowestfreq) * 0.305;
	shortelement = (492/highestfreq) * 0.305;
	
	wirex=0;		// start at one end of the boom
	wirey = longelement/2;
	
	e1 = LPDA(numElements,wirex, wirey, height);
	currentFeed( e1, 1.0, 0 ) ;
}

control()
{
	setFrequency(14.0);
	addFrequency(18.1);
	addFrequency(21.0);
	addFrequency(24.9);
	addFrequency(28.0);
	runModel();
}

//
// Create the longest element and then call LPDA2() for the rest of the elements
//
element LPDA( int numelements, real x, real y, real height)
{
	element e, er;
	e= wire( x, y, height, x, -y, height, 0.5", 21 ) ;
	er = LPDA2(e,numelements-1, x, y, height);	
	return er;
}

//
// Create all subsequent elements after the longest one and
// connect them with crossed transmission lines. This function
// calls itself to generate successive elements. It returns the
// shortest element which we also feed.
//
element LPDA2( element el, int numelements, real x, real y, real height)
{
	element e2,e3;
	x = x + boomdelta;
	y = y - eledelta;
	e2 = wire(x,y,height,x,-y,height,0.5",21);
	crossedTransmissionLine(el, e2, 50);		// it's probably higher Z than 50 ohms
	
	// Call ourself as long as there are more elements to create
	if (numelements > 1)
		e2 = LPDA2(e2, numelements-1, x, y, height);
				
	return e2;
}

