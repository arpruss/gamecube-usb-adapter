use <roundedsquare.scad>;

innerLength = 80;
extraWidth = 15;
sideWall = 1.5;
topWall = 1.6;
bottomWall = 2;
bottomScrewLength = 8;
topScrewLength = 5.7;
screwDiameter = 2.12;
screwHeadDiameter = 4.11;
thinPillarDiameter = 5.5;
fatPillarDiameter = 7;
stm32Width = 24.35;
stm32Length = 57.2;
tolerance = 0.2;

module dummy() {}

innerWidth = extraWidth+stm32Width+fatPillarDiameter*2+extraWidth;

module bottom(inset=false) {
    translate(inset?[0,0]:[-sideWall,-sideWall])
    roundedSquare([innerLength+inset?0:2*sideWall,innerWidth+inset?0:2*sideWall], radius=fatPillarDiameter/2-inset?sideWall:0);
}

bottom();