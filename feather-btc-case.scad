$fn = 50;
use <ruler.scad>
// translate([-1, -1, 0]) {
// 	%xyzruler(); 
// }

module topless_cube(box_dimensions, corner_radius) {
	difference() {
		// The outer box
		translate([corner_radius, corner_radius, corner_radius]) {
			minkowski() {
				// shrink cube to be "inside" the radius then add the roundness
				cube([(box_dimensions[0] - corner_radius * 2), 
					  (box_dimensions[1] - corner_radius * 2), 
					  (box_dimensions[2] - corner_radius)]);
				sphere(corner_radius);
			}
		}

		// Remove top of box
		// translate slice to top of model
		translate([0, 0, (box_dimensions[2])]) {
			// full size slice to take out
			cube(box_dimensions);
		}
	}
}

module hollow_box(outside_cube, corner_radius, wall_thickness) {
	difference() {
		// The outer box
		topless_cube(outside_cube, corner_radius);

		// The inner box (hollow part) inset by wall thickness
		translate([wall_thickness, wall_thickness, wall_thickness]) {
			inner_box = [(outside_cube[0] - wall_thickness * 2.0),
						 (outside_cube[1] - wall_thickness * 2.0),
						 (outside_cube[2])];
			topless_cube(inner_box, corner_radius);
		}
	}
}

module supports(box, wall_thickness) {
	// Supports for Feather w/10mm leads
	// support is outer_box - 2 * wall thickness - 2 * 5mm (where holes go)
	union() {
		spacing = 5;
		support = [box[0] - (2 * wall_thickness) - (2 * spacing),
				(2 * wall_thickness), 
				10];
		translate([wall_thickness + spacing, 
                    (wall_thickness * 2.5), 
                    wall_thickness]) {
			cube(support);
		}

		translate([wall_thickness + spacing, 
                    box[1]-(wall_thickness * 4.5), 
                    wall_thickness]) {
			cube(support);
		}
	}
}

module base_case(box, wall) {
	//topless_cube([56, 28, 30], 1, 2);
	// Make 56mm x 28mm x 30mm with 1mm rounded corners and 2mm thick walls
	// ...the inside will thus be 52mm x 24mm x 30mm
	difference() {
		// the box
		difference() {
			union() {
				hollow_box(box, 1, wall);

				// screw hole supports
				translate([wall + 2, wall + 2, wall-.5])
					cylinder((wall * 1.5), d=(wall * 2.5), center=true);
				translate([box[0] - (wall + 2), wall + 2, wall-.5])
					cylinder((wall * 1.5), d=(wall * 2.5), center=true);
				translate([wall + 2, box[1] - (wall + 2), wall-.5])
					cylinder((wall * 1.5), d=(wall * 2.5), center=true);
				translate([box[0] - (wall + 2), box[1] - (wall + 2), wall-.5])
					cylinder((wall * 1.5), d=(wall * 2.5), center=true);
			}

			// screw holes, 4 of them
			translate([wall + 2, wall + 2, -0.25]) {
				cylinder((wall * 4), d=wall, center=true);
				cylinder((wall * 0.75), wall);
			}
				
			translate([box[0] - (wall + 2), wall + 2, -0.25]) {
				cylinder((wall * 4), d=wall, center=true);
				cylinder((wall * 0.75), wall);
			}

			translate([wall + 2, box[1] - (wall + 2), -0.25]) {
				cylinder((wall * 4), d=wall, center=true);
				cylinder((wall * 0.75), wall);
			}

			translate([box[0] - (wall + 2), box[1] - (wall + 2), -0.25]) {
				cylinder((wall * 4), d=wall, center=true);
				cylinder((wall * 0.75), wall);
			}
		}

		// Hole for Mini-USB Plug
		translate([0, (box[1] / 2), 15]) {
			color("red") minkowski() {
				cube([wall * 4, 6, 2], center=true);
				sphere(0.5);
			}
		}

		translate([0, (box[1] / 2), 15]) {
			color("orange") minkowski() {
				cube([wall, 10, 6], center=true);
				sphere(0.5);
			}
		}
	}

	// Supports for Feather
	color("blue") supports(box, 2);
}

box = [56, 28, 33];
top = [56, 28, 4];
wall = 2;


base_case(box, wall);

// legs
translate([6, -8, 14.5]) {
    minkowski() {
        difference() {
            cube([wall - 1, 18, 27], center= true);
            translate([0, -19, 0]) rotate([15, 0, 0]) cube([10, 40, 40], center=true);
        }
        
        sphere(1);
    }
}

translate([49, -8, 14.5]) {
    minkowski() {
        difference() {
            cube([wall - 1, 18, 27], center= true);
            translate([0, -19, 0]) rotate([15, 0, 0]) cube([10, 40, 40], center=true);
        }
        
        sphere(1);
    }
}
