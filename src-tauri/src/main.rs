// #![cfg_attr(
//   all(not(debug_assertions), target_os = "windows"),
//   windows_subsystem = "windows"
// )]
// #![feature(slice_ptr_get)]
//#![allow(non_snake_case)]
#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![allow(unused_imports)]

include!(concat!(env!("OUT_DIR"), "/binding.rs"));

use lazy_static::lazy_static;
use once_cell::sync::Lazy;
use std::ffi::{CStr, CString};
use std::mem::MaybeUninit;
use std::os::raw;
use std::ptr;
use std::sync::{mpsc, Mutex};
use std::thread;
use std::time::Duration;
use serde::{Deserialize, Serialize};
use tauri::Manager;
use tauri::Window;
use tauri::{CustomMenuItem, SystemTray, SystemTrayEvent, SystemTrayMenu, SystemTrayMenuItem};


fn new_system_tray() -> SystemTray {
  let show_item = CustomMenuItem::new("show".to_string(), "Show");
  let hide_item = CustomMenuItem::new("hide".to_string(), "Hide");
  let separator_item = SystemTrayMenuItem::Separator;
  let quit_item = CustomMenuItem::new("quit".to_string(), "Quit");
  let tray_menu = SystemTrayMenu::new()
    .add_item(show_item)
    .add_item(hide_item)
    .add_native_item(separator_item)
    .add_item(quit_item);
  SystemTray::new().with_menu(tray_menu)
}

fn on_tray_menu_doubleclick(window: Window) {
  println!("Tray menu double-clicked.");
  window.show().unwrap();
}

fn on_tray_menu_item_click(window: Window, id: String) {
  match id.as_str() {
    "show" => {
      println!("Show menu item clicked.");
      window.show().unwrap();
    }
    "hide" => {
      println!("Hide menu item clicked.");
      window.hide().unwrap();
    }
    "quit" => {
      println!("Quit menu item clicked.");
      window.close().unwrap();
      std::process::exit(0);
    }
    _ => {
      println!("'{}' menu item clicked.", id);
    }
  }
}

fn hide_console_window() {
  use winapi::um::wincon::GetConsoleWindow;
  use winapi::um::winuser::{ShowWindow, SW_HIDE};

  let window = unsafe { GetConsoleWindow() };
  // https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindow
  if window != ptr::null_mut() {
    unsafe {
      ShowWindow(window, SW_HIDE);
    }
  }
}

//
// Globals
//
lazy_static! {
	static ref QUERY_INTERFACE: Mutex<VDP_SERVICE_QUERY_INTERFACE> =
		Mutex::new(VDP_SERVICE_QUERY_INTERFACE {
			Version: 0,
			QueryInterface: None,
		});
}

//
// Load vdpService.dll
//
static LIB_VDP_SERVICE: Lazy<libloading::Library> = Lazy::new(|| unsafe {
	libloading::Library::new("vdpService.dll").expect("XXX can't load the vdpService dll")
});

type Fn_VDPService_ServerInit = libloading::Symbol<'static, 
	unsafe extern "C" fn (
		token: *const ::std::os::raw::c_char,
		qi: *mut VDP_SERVICE_QUERY_INTERFACE,
		channelHandle: *mut *mut ::std::os::raw::c_void,
	) -> Bool >;
static Fn_VDPService_ServerInit: Lazy<Fn_VDPService_ServerInit> = Lazy::new(|| unsafe { LIB_VDP_SERVICE.get(b"VDPService_ServerInit").unwrap() });

//
// Load sideband_inputs_trigger.dll
//
static LIB_TRIGGER: Lazy<libloading::Library> = Lazy::new(|| unsafe {
	libloading::Library::new("sideband_inputs_trigger.dll").expect("XXX can't load the Trigger dll")
});

type FN_GO_ABOUT = libloading::Symbol<'static, unsafe extern "C" fn()>;
static GO_ABOUT: Lazy<FN_GO_ABOUT> = Lazy::new(|| unsafe { LIB_TRIGGER.get(b"GoAbout").unwrap() });

type FN_GO_TRIGGER = libloading::Symbol<'static, unsafe extern "C" fn(*mut raw::c_char) -> GoInt>;
static GO_TRIGGER: Lazy<FN_GO_TRIGGER> =
	Lazy::new(|| unsafe { LIB_TRIGGER.get(b"GoTrigger").unwrap() });

//
// GUIDs
//
const _GUID_VDPService_ChannelInterface_V1: GUID = GUID {
	field1: 0xA500A600,
	field2: 0x0006,
	field3: 0x81E2,
	field4: (0xc1 << 8) | 0x88,
	field5: [0x29, 0xA7, 0xD3, 0xA9, 0x3A, 0x62],
};

const _GUID_VDPService_ChannelInterface_V3: GUID = GUID {
	field1: 0xA500A600,
	field2: 0x000B,
	field3: 0x81E2,
	field4: (0xc1 << 8) | 0x88,
	field5: [0x29, 0xA7, 0xD3, 0xA9, 0x3A, 0x62],
};

const _GUID_VDPRPC_ChannelObjectInterface_V3: GUID = GUID {
	field1: 0xA500A600,
	field2: 0x000F,
	field3: 0x81E2,
	field4: (0xc1 << 8) | 0x88,
	field5: [0x29, 0xA7, 0xD3, 0xA9, 0x3A, 0x62],
};

const _GUID_VDPRPC_ChannelContextInterface_V2: GUID = GUID {
	field1: 0xA500A600,
	field2: 0x0008,
	field3: 0x81E2,
	field4: (0xc1 << 8) | 0x88,
	field5: [0x29, 0xA7, 0xD3, 0xA9, 0x3A, 0x62],
};

const _GUID_VDPRPC_VariantInterface_V1: GUID = GUID {
	field1: 0xA500A600,
	field2: 0x0000,
	field3: 0x81E2,
	field4: (0xc1 << 8) | 0x88,
	field5: [0x29, 0xA7, 0xD3, 0xA9, 0x3A, 0x62],
};

//
// Data structures
//
struct USER_DATA {
	inputs_tx: mpsc::Sender<CString>,
	chan_context_interface: VDPRPC_ChannelContextInterface,
	variant_interface: VDPRPC_VariantInterface,
}



#[tauri::command]
async fn sideband_func(window: Window) {
  //
  // Init
  //
    println!("** EagleRay Sideband Input Receiver! v1.2.0 (json, main channel)");
    unsafe { (*GO_ABOUT)() };
    let plugin_name = b"EagleRay\0".as_ptr() as *const raw::c_char;
    // Must match the plugin's name.
    let (inputs_tx, inputs_rx) = mpsc::channel::<CString>();
    //
    // GoTrigger() thread
    //
    thread::spawn(move || {
		    // let mut i = 0;

		    loop {
			    let param = inputs_rx.recv().expect("can't receive from inputs channel");

			    unsafe { (*GO_TRIGGER)(param.as_ptr() as *mut i8) };
			    // i += 1;
			    // println!("			param {}: {}", i, param.into_string().unwrap());
			    // thread::sleep(Duration::new(0, 10_000_000));
		    }
	    });
    //
    // Main infinite loop
    //
    loop {
		    let mut init = 0;
		    let mut qi_guard = QUERY_INTERFACE.lock().unwrap();
		    let mut chan_handle = ptr::null_mut();
		    let mut chan_interface;

		    //
		    // Wait for Channel connection
		    //
		    let mut i = 0;
		    loop {
			    // Init
			    let mut chan_interface_tmp = MaybeUninit::<VDPService_ChannelInterface>::uninit();
			    if init == 0 {
				    // Init Channel Handle
				    unsafe {
					    Fn_VDPService_ServerInit(plugin_name, &mut *qi_guard, &mut chan_handle);
				    }

				    // Get Channel Interface
				    unsafe {
					    qi_guard.QueryInterface.unwrap()(
						    &_GUID_VDPService_ChannelInterface_V3,
						    chan_interface_tmp.as_mut_ptr() as *mut raw::c_void,
					    );
				    }
			    }
			    chan_interface = unsafe { chan_interface_tmp.assume_init() };

			    // Connect
			    init = unsafe { chan_interface.v1.Connect.unwrap()() }; // To trigger the client's VDPService_PluginCreateInstance callbackk
			    println!("	Connect ({})   = {}", i, init);
          window.emit("event", &format!("    Connect ({})   = {}", i, init)).unwrap();
			    i += 1;

			    // Get Channel State
			    let r = unsafe { chan_interface.v1.GetChannelState.unwrap()() };
			    println!("	Channel state = {}", r);
          window.emit("event", &format!("    Channel state = {}", r)).unwrap();
			    if r == _VDPService_ChannelState_VDP_SERVICE_CHAN_CONNECTED {
				    break;
			    }

			    // Poll
			    println!("	Polling...");
          window.emit("event", &"    Polling...").unwrap();
			    unsafe { chan_interface.v1.Poll.unwrap()() }; // WARNING: Using the v3 version with any timeout may get stuck.
			    thread::sleep(Duration::new(1, 0)); // When there's no Horizon session, Poll() seems to block forever. Therefore, making this sleep short will shorten the connection time.
		    } // Wait for Channel connection

    //region
		    //
		    // Get Channel_Object_Interface
		    //
		    let mut chan_obj_interface = MaybeUninit::<VDPRPC_ChannelObjectInterface>::uninit();
		    unsafe {
			    qi_guard.QueryInterface.unwrap()(
				    &_GUID_VDPRPC_ChannelObjectInterface_V3,
				    chan_obj_interface.as_mut_ptr() as *mut raw::c_void,
			    );
		    }
		    let chan_obj_interface = unsafe { chan_obj_interface.assume_init() };

		    //
		    // Get Channel_Context_Interface
		    //
		    let mut chan_context_interface = MaybeUninit::<VDPRPC_ChannelContextInterface>::uninit();
		    unsafe {
			    qi_guard.QueryInterface.unwrap()(
				    &_GUID_VDPRPC_ChannelContextInterface_V2,
				    chan_context_interface.as_mut_ptr() as *mut raw::c_void,
			    );
		    }
		    let chan_context_interface = unsafe { chan_context_interface.assume_init() };

		    //
		    // Get Variant_Interface
		    //
		    let mut variant_interface = MaybeUninit::<VDPRPC_VariantInterface>::uninit();
		    unsafe {
			    qi_guard.QueryInterface.unwrap()(
				    &_GUID_VDPRPC_VariantInterface_V1,
				    variant_interface.as_mut_ptr() as *mut raw::c_void,
			    );
		    }
		    let variant_interface = unsafe { variant_interface.assume_init() };

		    //
		    // Drop Query-Interface
		    //
		    std::mem::drop(qi_guard); // unlock the global mutex to allow other callbacks to use while this thread is polling.

		    //
		    // Create a channel object
		    //
		    let chan_obj_name = b"ERay_Input\0".as_ptr() as *const raw::c_char; // Must match the plugin's channel object name.
		    let obj_notify_sink = VDPRPC_ObjectNotifySink {
			    version: 1,
			    v1: _VDPRPC_ObjectNotifySink__bindgen_ty_1 {
				    OnInvoke: Some(OnInvoke),
				    OnObjectStateChanged: Some(OnObjectStateChanged),
			    },
		    };
		    let mut chan_obj_handle: *mut raw::c_void = ptr::null_mut();
		    let mut user_data = USER_DATA {
			    inputs_tx: inputs_tx.clone(),
			    chan_context_interface,
			    variant_interface,
		    };

		    let r = unsafe {
			    chan_obj_interface.v1.CreateChannelObject.unwrap()(
				    chan_obj_name,
				    &obj_notify_sink,
				    &mut user_data as *mut _ as *mut raw::c_void, // ptr::null_mut(),
				    _VDPRPC_ObjectConfigurationFlags_VDP_RPC_OBJ_CONFIG_DEFAULT, // | _VDPRPC_ObjectConfigurationFlags_VDP_RPC_OBJ_CONFIG_INVOKE_ALLOW_ANY_THREAD,
				    &mut chan_obj_handle,
			    )
		    };
		    println!("	Create Channel Object: {}", r);

		    // //
		    // // Wait to be connected
		    // //
		    // println!("	Wait for VDP_RPC_OBJ_CONNECTED...");
		    // loop {
		    // 	unsafe { chan_interface.v3.Poll.unwrap()( 999 ) };		// in milliseconds
		    // 	let chan_obj_state = unsafe { chan_obj_interface.v1.GetObjectState.unwrap()(chan_obj_handle) };
		    // 	if chan_obj_state == _VDPRPC_ObjectState_VDP_RPC_OBJ_CONNECTED {
		    // 		break;
		    // 	}
		    // }

		    // //
		    // // Request Side Channel
		    // //
		    // let side_chan_name = b"ERay_Side\0".as_ptr() as *const raw::c_char;											// Must match the peer's side channel name.
		    // let r = unsafe { chan_obj_interface.v2.
		    // 	RequestSideChannel.unwrap()(chan_obj_handle, _VDPRPC_SideChannelType_VDP_RPC_SIDE_CHANNEL_TYPE_PCOIP, side_chan_name) };
		    // println!("	Request side channel: {}", r);

		    // //
		    // // Wait to be connected
		    // //
		    // println!("	Wait for VDP_RPC_OBJ_SIDE_CHANNEL_CONNECTED...");
		    // loop {
		    // 	unsafe { chan_interface.v1.Poll.unwrap()() };			// There may be a WEIRD TIMING ISSUE.
		    // 	let chan_obj_state = unsafe { chan_obj_interface.v1.GetObjectState.unwrap()(chan_obj_handle) };
		    // 	if chan_obj_state == _VDPRPC_ObjectState_VDP_RPC_OBJ_SIDE_CHANNEL_CONNECTED {
		    // 		break;
		    // 	}
		    // }

		    //
		    // Wait for Invoke
		    //
		    loop {
			    println!("	Wait for Invoke...");
			    unsafe { chan_interface.v3.Poll.unwrap()(999_999) }; // in milliseconds

			    // Get Channel State
			    let r = unsafe { chan_interface.v1.GetChannelState.unwrap()() };
			    if r != _VDPService_ChannelState_VDP_SERVICE_CHAN_CONNECTED {
				    println!("	Channel state = {}", r);
				    break;
			    }
		    }

		    //
		    // Destroy Channel Object
		    //
		    let r = unsafe { chan_obj_interface.v1.DestroyChannelObject.unwrap()(chan_obj_handle) };
		    println!("	Destroy channel object: {}", r);
	    }
}

#[no_mangle]
pub extern "C" fn OnInvoke(
	userData: *mut raw::c_void,
	contextHandle: *mut raw::c_void,
	_reserved: *mut raw::c_void,
) {
	// println!("				* On-Invoke()");
	let user_data = userData as *const USER_DATA;

	// Debug
	let command =
		unsafe { (*user_data).chan_context_interface.v1.GetCommand.unwrap()(contextHandle) };
	let count = unsafe {
		(*user_data)
			.chan_context_interface
			.v1
			.GetParamCount
			.unwrap()(contextHandle)
	};
	println!("		*** Command: {}, Param count: {}", command, count);

	//
	// Get the string Variant
	//
	let mut variant = MaybeUninit::<VDP_RPC_VARIANT>::uninit();
	let r = unsafe {
		(*user_data).chan_context_interface.v1.GetParam.unwrap()(
			contextHandle,
			0,
			variant.as_mut_ptr(),
		)
	};
	if r == 0 {
		println!("                                              *** Error in On_Invoke(): Command: {}", command);
		return;
	}
	let mut variant = unsafe { variant.assume_init() };

	//
	// Trigger
	//
	let param = unsafe { CStr::from_ptr(variant.__bindgen_anon_1.strVal) }.to_owned();
	unsafe {
		(*user_data)
			.inputs_tx
			.send(param)
			.expect("				XXX can't send param")
	};

	//
	// Clean up
	//
	let _ = unsafe { (*user_data).variant_interface.v1.VariantClear.unwrap()(&mut variant) };
}

//
// On_Object_State_Changed
//	Exported callback for VdpService
//
#[no_mangle]
pub extern "C" fn OnObjectStateChanged(_userData: *mut raw::c_void, _reserved: *mut raw::c_void) {
	println!("\n				* On-Object-State-Changed()");
}

//
// Tests
//
// #[cfg(test)]
// mod tests {

// 	#[link(name = "_ERay_sideband_inputs_sender")]
// 	extern "C" {
// 		fn VDPService_PluginInit(qi: VDP_SERVICE_QUERY_INTERFACE) -> Bool;
// 	}

// 	#[test]
// 	fn it_works() {
// 		assert_eq!(2 + 2, 4);
// 		unsafe {
// 			VDPService_PluginInit(VDP_SERVICE_QUERY_INTERFACE {
// 				Version: 0,
// 				QueryInterface: None,
// 			});
// 		}
// 	}
// }

//
// Main
//
fn main() {
  hide_console_window();
  let context = tauri::generate_context!();

  tauri::Builder::default()
  .invoke_handler(tauri::generate_handler![sideband_func])
  .system_tray(new_system_tray())
  .on_system_tray_event(|app, event| {
    let window = app.get_window("main").unwrap();
    match event {
      SystemTrayEvent::DoubleClick {
        position: _,
        size: _,
        ..
      } => on_tray_menu_doubleclick(window),
      SystemTrayEvent::MenuItemClick { id, .. } => on_tray_menu_item_click(window, id),
      _ => {}
    }
  })
  .run(context)
  .expect("error while running tauri application");
}