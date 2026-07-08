
// Pulsador momentáneo NC - modelo aproximado basado en fotografías
$fn=80;

module button(){
    color("red")
    translate([0,0,15])
        cylinder(h=3,d=7);
}

module body(){
    color("silver"){
        cylinder(h=15,d=6);
        translate([0,0,4])
            cylinder(h=7,d=6.0);
    }
}

module nut(){
    color("silver")
    translate([0,0,4])
        cylinder(h=2,d=10,$fn=6);
}

module washer(){
    color("silver")
    translate([0,0,3.4])
    difference(){
        cylinder(h=0.6,d=8);
        cylinder(h=0.7,d=6.2);
    }
}

module insulator(){
    color("black")
    translate([0,0,-8])
        cylinder(h=8,d=6);
}

module terminals(){
    color("gold"){
        translate([-1.5,0,-15])
            cube([1,0.8,7]);
        translate([0.5,0,-15])
            cube([1,0.8,7]);
    }
}

body();
button();
washer();
nut();
insulator();
terminals();
