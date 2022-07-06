mod sideband;
use tauri::Window;
use tauri::Manager;
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
  use std::ptr;
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

fn main() {
  hide_console_window();
  let context = tauri::generate_context!();

  tauri::Builder::default()
  .invoke_handler(tauri::generate_handler![sideband::sideband_func])
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