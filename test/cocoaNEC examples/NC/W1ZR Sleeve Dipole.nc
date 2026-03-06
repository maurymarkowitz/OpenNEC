//	Ref. 2011.05 QST pg. 58
//	Author: Willard Myers, K1GQ.
//	Dimensions, wire and insulation properties from EZNEC model by Joel, W1ZR.

model("W1ZR Skeleton Sleeve Dipole")
{
	real r;
	real wireSigma;
	real insulationPermittivity;
	real insulationRadius;
	real l1, l2, l3, s;
	real h;
	element wire1, wire2, wire3, wire4, wire5, wire6, wire7;
	
	r = #18;
	wireSigma = 5.7471E7;
	insulationPermittivity = 2.25;
	insulationRadius = 2.074E-3; 
	
	l1 = (30.9') / 2; // QST says 30.833'
	l2 = (55.9') / 2; // QST says 56.333'
	l3 = l1 + 4.2";   // QST say gap 4"
	s = 0.72";
	
	h = 25';
	
	wire1 = wire(-l1, 0, h, l1, 0, h, r, 102);
	
	wire2 = wire(-l2, 0, h - s, l2, 0, h - s, r, 183);
	
	wire3 = wire(-l2, 0, h, -l3, 0, h, r, 63);
	wire4 = wire( l2, 0, h,  l3, 0, h, r, 63);
	
	wire5 = wire(-l2, 0, h, -l2, 0, h - s, r, 1);
	wire6 = wire( l2, 0, h,  l2, 0, h - s, r, 1);
	
	conductivity(wire1, wireSigma);
	conductivity(wire2, wireSigma);
	conductivity(wire3, wireSigma);
	conductivity(wire4, wireSigma);
	
	insulate(wire1, insulationPermittivity, 0, insulationRadius);
	insulate(wire2, insulationPermittivity, 0, insulationRadius);
	insulate(wire3, insulationPermittivity, 0, insulationRadius);
	insulate(wire4, insulationPermittivity, 0, insulationRadius);
		
	voltageFeed(wire2, 1, 0);
	
	frequencySweep(7.0, 7.3, 7);
	frequencySweep(14.0, 14.35, 8);
	
	averageGround();
}
