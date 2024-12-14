#include <lvgl.h>
#include "common.h"
#include <Arduino.h>
#include "driver/lv_port_disp.h"
#include "driver/lv_port_indev.h"
#include "driver/lv_port_fatfs.h"

#if LV_BUILD_EXAMPLES
/*If you want to use the LVGL examples,
  make sure to install the lv_examples Arduino library
  and uncomment the following line.
  */
#include "examples/lv_examples.h"
#endif



// #include <lv_demo.h>
#include "demos/lv_demos.h"

#include "myGuiStudy.h"
#include "study/myTest.h"


//声明函数
void rtu_pages_memory_leak_test(void);
void lv_mem_usage_print(lv_mem_monitor_t *mem);
void fatfs_test();
void my_error_handling();

#if LV_USE_LOG != 0
/* Serial debugging */
// void my_print( lv_log_level_t level, const char * file, uint32_t line, const char * fn_name, const char * dsc )
// {
//     Serial.printf( "%s(%s)@%d->%s\r\n", file, fn_name, line, dsc );
//     Serial.flush();
// }
void my_log_cb(const char * buf)
{
  Serial.printf(buf, strlen(buf));
}
#endif


void setup()
{
    Serial.begin( 115200 ); /* prepare for possible serial debug */

    // pinMode(TFT_BL, OUTPUT);
    // digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
    // Serial.println( "test bl" );
    // while(1);

    String LVGL_Arduino = "Hello Arduino! ";
    LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch() ;

    Serial.println( LVGL_Arduino );
    Serial.println( "I am LVGL_Arduino" );


    #ifdef __cplusplus
    Serial.println( "__cplusplus" );
    #else
    Serial.println( "__c" );
    #endif

    lv_init();
    // sd_init();
    // lv_fs_fatfs_init();

    // fatfs_test();       //测试文件系统读取SD

#if LV_USE_LOG != 0
    lv_log_register_print_cb( my_log_cb ); /* register print function for debugging */
#endif

    tft.begin();          /* TFT init */
    tft.setRotation( 0 ); /* Landscape orientation, flipped */
#if USE_DMA
    tft.initDMA();
#endif

    lv_port_disp_init();        //显示设备初始化
    // lv_port_indev_init();       //初始化输入设备，这里是触摸屏或者拨动按键

#if 0
    /* Create simple label */
    rtu_pages_memory_leak_test();

    my_test();


    rtu_pages_memory_leak_test();

#else
    /* Try an example from the lv_examples Arduino library
       make sure to include it as written above.
    lv_example_btn_1();
   */

    rtu_pages_memory_leak_test();
    // uncomment one of these demos
    
    lv_demo_widgets();            // OK
    // Serial.println(LV_TEST_1);
    
    
    //lv_demo_benchmark();          // OK
    // lv_demo_keypad_encoder();     // works, but I haven't an encoder
    //lv_demo_music();              // 
    //lv_demo_printer();
    //lv_demo_stress();             // seems to be OK

    //lv_example_ime_pinyin_1();    //拼音输入法
    // lv_example_grid_1();
    //lv_example_get_started_1();
    //test_test1();
    // lv_example_meter_3();

    // lv_example_bar_6();
    // lv_example_meter_3();

    rtu_pages_memory_leak_test();
#endif
    Serial.println( "Setup done" );
}

void loop()
{
    lv_timer_handler(); /* let the GUI do its work */
    delay( 5 );
    // LV_LOG_USER("slider_value=%d",slider_value);
   

}

void fatfs_test()
{
    lv_fs_file_t f;
    lv_fs_res_t res;
    res = lv_fs_open(&f, "S:test/test.txt", LV_FS_MODE_RD);
    if(res != LV_FS_RES_OK) my_error_handling();

    uint32_t read_num;
    uint8_t buf[20];
    res = lv_fs_read(&f, buf, 20, &read_num);
    if(res != LV_FS_RES_OK || read_num != 20) my_error_handling();

    lv_fs_close(&f);

    Serial.print("read test.txt:");
    for(int i = 0 ; i < 20 && buf[i] != '\0'; i++)
    {
        Serial.write(buf[i]);
    }
    
    Serial.print("\r\n");
}

void my_error_handling()
{
    Serial.println("my_error_handling");
}

void lv_mem_usage_print(lv_mem_monitor_t *mem)
{
 
  Serial.printf("-- lv memory usage --\r\n");
 
  Serial.printf("  -- Total heap size--%d\r\n", mem->total_size);
 
  Serial.printf("  -- free_cnt--%d\r\n", mem->free_cnt);
 
  Serial.printf("  -- Size of available memory--%d\r\n", mem->free_size);
 
  Serial.printf("  -- free_biggest_size--%d\r\n", mem->free_biggest_size);
 
  Serial.printf("  -- used_cnt--%d\r\n", mem->used_cnt);
 
  Serial.printf("  -- Max size of Heap memory used--%d\r\n", mem->max_used);
 
  Serial.printf("  -- Percentage used--%d%%\r\n", mem->used_pct);
 
  Serial.printf("  -- Amount of fragmentation--%d\r\n", mem->frag_pct);
 
}
 
 
void rtu_pages_memory_leak_test(void)
{
 
  Serial.printf("\r\n");
 
  Serial.printf("  *  rtu_pages_memory_leak_test  *  \r\n");
 
  Serial.printf("------------------------------------\r\n");
 
 
 
  lv_mem_monitor_t mon;
 
  lv_mem_monitor(&mon);
 
  lv_mem_usage_print(&mon);
 
}
