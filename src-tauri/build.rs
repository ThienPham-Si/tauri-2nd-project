extern crate bindgen;

use std::env;
use std::path::PathBuf;

fn main() {
	// Tell cargo to invalidate the built crate whenever the wrapper changes
	println!("cargo:rerun-if-changed=binding.hpp");

	// The bindgen::Builder is the main entry point
	// to bindgen, and lets you build up options for
	// the resulting bindings.
	let bindings = bindgen::Builder::default()
		// The input header we would like to generate
		// bindings for.
		.header("binding.hpp")
		// Tell cargo to invalidate the built crate whenever any of the
		// included header files changed.
		.parse_callbacks(Box::new(bindgen::CargoCallbacks))
		// Finish the builder and generate the bindings.
		.generate()
		// Unwrap the Result and panic on failure.
		.expect("Unable to generate bindings");

	// Write the bindings to the $OUT_DIR/bindings.rs file.
	let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
	bindings
		.write_to_file(out_path.join("binding.rs"))
		.expect("Couldn't write bindings!");

	// Link to Sideband_Sender
	// println!("cargo:rustc-link-lib=dylib=_eray_sideband_inputs_sender");
	// println!("cargo:rustc-link-search=native=../Sideband_Inputs_Sender/target/debug");

	// // Link to vdpService.dll
	// println!("cargo:rustc-link-lib=dylib=vdpservice");
	// println!("cargo:rustc-link-search=native=../vdpservice/lib/win32/x64/debug");
    
    // Uncomment the following when compiling receiver on Linux host
    // println!("cargo:rustc-link-search=native=/usr/lib/pcoip/vchan_plugins/");
    // println!("cargo:rustc-env=LD_LIBRARY_PATH=/usr/lib/pcoip/vchan_plugins/");

}
