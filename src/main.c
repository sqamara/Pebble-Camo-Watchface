#include <pebble.h>

#define KEY_TEMPERATURE 0
#define KEY_CONDITIONS 1

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_weather_layer;
static Layer *s_simple_bg_layer;
// fonts
static GFont s_time_font;
static GFont s_weather_font;

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Store incoming information
  static char temperature_buffer[8];
  static char conditions_buffer[32];
  static char weather_layer_buffer[32];

  // Read tuples for data
  Tuple *temp_tuple = dict_find(iterator, KEY_TEMPERATURE);
  Tuple *conditions_tuple = dict_find(iterator, KEY_CONDITIONS);

  // If all data is available, use it
  APP_LOG(APP_LOG_LEVEL_DEBUG, "got the callback but");
//   APP_LOG(APP_LOG_LEVEL_DEBUG, "%dF", (int)temp_tuple->value->int32);
   APP_LOG(APP_LOG_LEVEL_DEBUG, "%s", conditions_tuple->value->cstring);

  if(temp_tuple && conditions_tuple) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Stting the buffer and is now working");
    snprintf(temperature_buffer, sizeof(temperature_buffer), "%dF", (int)temp_tuple->value->int32);
    snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", conditions_tuple->value->cstring);

    // Assemble full string and display
    snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s, %s", temperature_buffer, conditions_buffer);
    text_layer_set_text(s_weather_layer, weather_layer_buffer);
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void bg_update_proc(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorRed);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, GColorWhite);
  
  GColor prev_color = GColorBlack;
  for (int x = 0; x < 144; x+=12) {
      for (int y = 0; y < 168; y+=12) {
        GPath *s_my_path_ptr;
        GPathInfo BOLT_PATH_INFO;
        BOLT_PATH_INFO.num_points = 4;
        BOLT_PATH_INFO.points = (GPoint []) {{x, y}, {x+12, y}, {x+12, y+12}, {x, y+12}};
        int decider = rand() % 7;
        GColor color;
        switch (decider) {
          case 0:
            color = GColorArmyGreen; 
            break;
          case 1:
            color = GColorBrass; 
            break;
          case 2:
            color = GColorBlack; 
            break;
          case 3:
            color = GColorWindsorTan; 
            break;
          default:
            color = prev_color;
      }
      prev_color = color;
    s_my_path_ptr = gpath_create(&BOLT_PATH_INFO);
    graphics_context_set_fill_color(ctx, color);
    gpath_draw_filled(ctx, s_my_path_ptr);    
    graphics_context_set_stroke_color(ctx, color);
    gpath_draw_outline(ctx, s_my_path_ptr);
    gpath_destroy(s_my_path_ptr);
    }
  }
}

static void main_window_load(Window *window) {
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  //crate the background
  s_simple_bg_layer = layer_create(bounds);
  layer_set_update_proc(s_simple_bg_layer, bg_update_proc);
  layer_add_child(window_layer, s_simple_bg_layer);
  
  //initialize custom fonts
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BLACK_OPS_44));
  s_weather_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BLACK_OPS_20));

  
  // Create the TextLayer with specific bounds
  s_time_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w, 50));

  // Improve the layout to be more like a watchface
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_font(s_time_layer, s_time_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  
  
  // Create temperature Layer
  s_weather_layer = text_layer_create(
    GRect(0, 120, bounds.size.w, 30));

  // Style the text
  text_layer_set_background_color(s_weather_layer, GColorClear);
  text_layer_set_text_color(s_weather_layer, GColorWhite);
  text_layer_set_text(s_weather_layer, "Loading");
  text_layer_set_font(s_weather_layer, s_weather_font);
  text_layer_set_text_alignment(s_weather_layer, GTextAlignmentCenter);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_weather_layer));

}

static void main_window_unload(Window *window) {
  // Destroy TextLayer
  fonts_unload_custom_font(s_time_font);
  text_layer_destroy(s_time_layer);
  // Destroy weather elements
  text_layer_destroy(s_weather_layer);
  fonts_unload_custom_font(s_weather_font);
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  
  if(tick_time->tm_min % 30 == 0) {
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    // Add a key-value pair
    dict_write_uint8(iter, 0, 0);

    // Send the message!
    app_message_outbox_send();
  }
}

static void init() {  
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  // Make sure the time is displayed from the start
  update_time();  
  
  // Register with TickTimerService
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
  layer_destroy(s_simple_bg_layer);

}

int main(void) {
  init();
  app_event_loop();
  deinit();
}



