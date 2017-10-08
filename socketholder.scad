use <roundedsquare.scad>;

sizeX=10;
sizeY=13.06;
wall=1.3;
holeX=5.5;
holeY=3;
tolerance=0.3;
length=25;

module dummy() {};

cX = sizeX/2+tolerance;
cY = sizeY/2+tolerance;

render(convexity=2)
difference() {
    translate([-wall,-wall,0]) linear_extrude(height=length) roundedSquare([sizeX+2*tolerance+2*wall,sizeY+2*tolerance+2*wall]);
    translate([0,0,wall]) cube([sizeX+2*tolerance,sizeY+2*tolerance,length]);
    translate([cX,cY-sizeY/3]) cube([holeX+2*tolerance,holeY+2*tolerance,4*wall],center=true);
    translate([cX,cY+sizeY/3]) cube([holeX+2*tolerance,holeY+2*tolerance,4*wall],center=true);
}

