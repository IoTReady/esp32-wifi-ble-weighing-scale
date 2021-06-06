//
//  console_lib.c
//  Weighing_Scale_Firmware
//
//  ESP32 firmware for Smart Weighing Scale
//
//  This module provides access to run-time inputs via. keyboard.
//
//  Created by IoTReady Technology Solutions Pvt. Ltd., Bangalore
//  Copyright © 2019 IoTReady Technology Solutions Pvt. Ltd., Bangalore
//
// All rights reserved. Gaining access to this code without explicit 
// license or through reverse compilation constitutes violation of 
// protected intellectual property rights.
// 
// If you are interested in using this code or similar functionality
// for your product, please contact us at hello@iotready.co
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "esp_log.h"
#include "esp_console.h"
#include "esp_system.h"
#include "esp_sleep.h"
#include "esp_spi_flash.h"
#include "driver/rtc_io.h"
#include "driver/uart.h"
#include "argtable3/argtable3.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/rtc_cntl_reg.h"
#include "rom/uart.h"
#include "console_lib.h"
#include "sdkconfig.h"
#include "HX711.h"
#include "weighing_scale.h"
#include "nvs_store.h"
#include "esp_vfs_dev.h"
#include "linenoise/linenoise.h"
#include "battery_level_check.h"

#ifdef CONFIG_FREERTOS_USE_STATS_FORMATTING_FUNCTIONS
    #define WITH_TASKS_INFO 1
#endif

float test_calibrate;
int linechar;

static const char *TAG = "cmd_system";

static void register_to_tare();
static void register_menu();
static void register_cal_or_tare();
static void register_toExit();
static void register_to_calibrate();
static void register_new_calibrate();
static void register_save_new_calibrate();
#if WITH_TASKS_INFO
static void register_tasks();
#endif

void register_system()
{
    register_to_tare();
    register_menu();
    register_cal_or_tare();
    register_toExit();
    register_to_calibrate();
    register_new_calibrate();
    register_save_new_calibrate();
#if WITH_TASKS_INFO
    register_tasks();
#endif
}

static int menu(int argc, char **argv)
{
    vTaskSuspend(weighingTask_Handle);
    vTaskSuspend(battery_level_check_task_handle);
    ESP_LOGI(TAG,"SENSOR CALIBRATION!\n");
    vTaskDelay(100 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG,"MAIN MENU \n");
    ESP_LOGI(TAG,"------------ \n");
    ESP_LOGI(TAG,"Please select a sensor number from the options available\n\n");
    ESP_LOGI(TAG,"1. Sensor 1 \nx. Exit\n\n\n");
    const char *prompt = LOG_COLOR_I "IoTReady " LOG_RESET_COLOR;
    char* line = linenoise(prompt);
    int ret;
    esp_err_t err = esp_console_run(line, &ret);
        if (err == ESP_ERR_NOT_FOUND) {
              ESP_LOGI(TAG,"Unrecognized command\n");
        } else if (err == ESP_ERR_INVALID_ARG) {
            // command was empty
        } else if (err == ESP_OK && ret != ESP_OK) {
              ESP_LOGI(TAG,"Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(err));
        } else if (err != ESP_OK) {
              ESP_LOGI(TAG,"Internal error: %s\n", esp_err_to_name(err));
        }
        /* linenoise allocates line buffer on the heap, so need to free it */
        linenoiseFree(line);
    return 0;
}

static void register_menu()
{
        const esp_console_cmd_t cmd = {
        .command = "m",
        .help = NULL,
        .hint = NULL,
        .func = &menu,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

static int toExit(int argc, char** argv)
{
    vTaskResume(weighingTask_Handle);
    vTaskResume(battery_level_check_task_handle);    
    return 0;
}

static void register_toExit()
{
    const esp_console_cmd_t cmd = {
        .command = "x",
        .help = NULL,
        .hint = NULL,
        .func = &toExit,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

static int cal_or_tare(int args, char **argv)
{
      ESP_LOGI(TAG,"\nEnter c for calibration\n");
      ESP_LOGI(TAG,"Enter t for tare\n");
    const char* prompt = LOG_COLOR_I "IoTReady> " LOG_RESET_COLOR;
    char* line = linenoise(prompt);
     /* Try to run the command */
        int ret;
        esp_err_t err = esp_console_run(line, &ret);
        if (err == ESP_ERR_NOT_FOUND) {
              ESP_LOGI(TAG,"Unrecognized command\n");
        } else if (err == ESP_ERR_INVALID_ARG) {
            // command was empty
        } else if (err == ESP_OK && ret != ESP_OK) {
              ESP_LOGI(TAG,"Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(err));
        } else if (err != ESP_OK) {
              ESP_LOGI(TAG,"Internal error: %s\n", esp_err_to_name(err));
        }
        /* linenoise allocates line buffer on the heap, so need to free it */
        linenoiseFree(line);
    return 0;
}

static void register_cal_or_tare()
{
    const esp_console_cmd_t cmd = {
        .command = "1",
        .help = NULL,
        .hint = NULL,
        .func = &cal_or_tare,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

static int to_tare(int argc, char **argv)
{
      ESP_LOGI(TAG,"OFFSET before tare : %ld\n",OFFSET);
    tare(times);
    write_offset_nvs();
      ESP_LOGI(TAG,"OFFSET after tare: %ld\n", OFFSET);
      ESP_LOGI(TAG,"Press x to exit\n");
    return 0;
}

static void register_to_tare()
{
    const esp_console_cmd_t cmd = {
        .command = "t",
        .help = NULL,
        .hint = NULL,
        .func = &to_tare,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

static int to_calibrate(int argc, char **argv)
{
      ESP_LOGI(TAG,"Current calibration factor: %ld\n", SCALE);
      ESP_LOGI(TAG,"Reading for the current calibration factor: %f\n", get_units(5));
      ESP_LOGI(TAG,"Enter sc to enter new calibration factor. \n");
      ESP_LOGI(TAG,"Press x to exit \n");
    const char* prompt = LOG_COLOR_I "IoTReady> " LOG_RESET_COLOR;
    char* line = linenoise(prompt);
     /* Try to run the command */
        int ret;
        esp_err_t err = esp_console_run(line, &ret);
        if (err == ESP_ERR_NOT_FOUND) {
              ESP_LOGI(TAG,"Unrecognized command\n");
        } else if (err == ESP_ERR_INVALID_ARG) {
            // command was empty
        } else if (err == ESP_OK && ret != ESP_OK) {
              ESP_LOGI(TAG,"Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(err));
        } else if (err != ESP_OK) {
              ESP_LOGI(TAG,"Internal error: %s\n", esp_err_to_name(err));
        }
        /* linenoise allocates line buffer on the heap, so need to free it */
        linenoiseFree(line);
    return 0;
}

static void register_to_calibrate()
{
    const esp_console_cmd_t cmd = {
        .command = "c",
        .help = NULL,
        .hint = NULL,
        .func = &to_calibrate,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

static int new_calibrate(int argc, char **argv)
{
      ESP_LOGI(TAG,"Enter the new calibration factor. \n");
    const char* prompt = LOG_COLOR_I "IoTReady> " LOG_RESET_COLOR;
    char* line = linenoise(prompt);
    linechar = atoi(line);
    test_calibrate = get_value(5)/linechar;
      ESP_LOGI(TAG,"Reading for the new calibration factor: %.3f\n", test_calibrate);
      ESP_LOGI(TAG,"Press \"save\" to save this calibration factor. \n");
      ESP_LOGI(TAG,"Press \"sc\" to enter another calibration factor. \n");
      ESP_LOGI(TAG,"Press \"x\" to exit.\n");
    linenoiseFree(line);
    return 0;
}

static void register_new_calibrate()
{
    const esp_console_cmd_t cmd = {
        .command = "sc",
        .help = NULL,
        .hint = NULL,
        .func = &new_calibrate,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

static int save_new_calibrate(int argc, char **argv)
{
    set_scale(linechar);
    write_scale_nvs();
    vTaskResume(weighingTask_Handle);
    vTaskResume(battery_level_check_task_handle);
    return 0;
}

static void register_save_new_calibrate()
{
    const esp_console_cmd_t cmd = {
        .command = "save",
        .help = NULL,
        .hint = NULL,
        .func = &save_new_calibrate,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

void initialize_console()
{
    /* Disable buffering on stdin and stdout */
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);

    /* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
    esp_vfs_dev_uart_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
    /* Move the caret to the beginning of the next line on '\n' */
    esp_vfs_dev_uart_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);

    /* Configure UART. Note that REF_TICK is used so that the baud rate remains
     * correct while APB frequency is changing in light sleep mode.
     */
    const uart_config_t uart_config = {
        .baud_rate = CONFIG_CONSOLE_UART_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .use_ref_tick = true};
    ESP_ERROR_CHECK(uart_param_config(CONFIG_CONSOLE_UART_NUM, &uart_config));

    /* Install UART driver for interrupt-driven reads and writes */
    ESP_ERROR_CHECK(uart_driver_install(CONFIG_CONSOLE_UART_NUM,
                                        256, 0, 0, NULL, 0));

    /* Tell VFS to use UART driver */
    esp_vfs_dev_uart_use_driver(CONFIG_CONSOLE_UART_NUM);

    /* Initialize the console */
    esp_console_config_t console_config = {
        .max_cmdline_args = 8,
        .max_cmdline_length = 256,
#if CONFIG_LOG_COLORS
        .hint_color = atoi(LOG_COLOR_CYAN)
#endif
    };
    ESP_ERROR_CHECK(esp_console_init(&console_config));

    /* Configure linenoise line completion library */
    /* Enable multiline editing. If not set, long commands will scroll within
     * single line.
     */
    linenoiseSetMultiLine(1);

    /* Tell linenoise where to get command completions and hints */
    linenoiseSetCompletionCallback(&esp_console_get_completion);
    linenoiseSetHintsCallback((linenoiseHintsCallback *)&esp_console_get_hint);

    /* Set command history size */
    linenoiseHistorySetMaxLen(100);

#if CONFIG_STORE_HISTORY
    /* Load command history from filesystem */
    linenoiseHistoryLoad(HISTORY_PATH);
#endif
}

void run_console(void *pvParameter)
{

    /* Register commands */
    esp_console_register_help_command();
    register_system();

    /* Prompt to be printed before each line.
     * This can be customized, made dynamic, etc.
     */
    const char *prompt = LOG_COLOR_I "IoTReady> " LOG_RESET_COLOR;

    /* Figure out if the terminal supports escape sequences */
    int probe_status = linenoiseProbe();
    if (probe_status)
    { /* zero indicates success */
          ESP_LOGI(TAG,"\n"
               "Your terminal application does not support escape sequences.\n"
               "Line editing and history features are disabled.\n"
               "On Windows, try using Putty instead.\n");
        linenoiseSetDumbMode(1);
// #if CONFIG_LOG_COLORS
//         /* Since the terminal doesn't support escape sequences,
//          * don't use color codes in the prompt.
//          */
//         prompt = "esp32> ";
// #endif //CONFIG_LOG_COLORS
    }

    /* Main loop */
    while (true)
    {
         
        /* Get a line using linenoise.
         * The line is returned when ENTER is pressed.
         */
        char *line = linenoise(prompt);
        if (line == NULL)
        { /* Ignore empty lines */
            continue;
        }
          ESP_LOGI(TAG,"line = %s \n", line);
        /* Add the command to the history */
                linenoiseHistoryAdd(line);
        #if CONFIG_STORE_HISTORY
                /* Save command history to filesystem */
                linenoiseHistorySave(HISTORY_PATH);
        #endif

        /* Try to run the command */
        int ret;
        esp_err_t err = esp_console_run(line, &ret);
        if (err == ESP_ERR_NOT_FOUND) {
                ESP_LOGI(TAG,"Unrecognized command\n");
        } else if (err == ESP_ERR_INVALID_ARG) {
            // command was empty
        } else if (err == ESP_OK && ret != ESP_OK) {
                ESP_LOGI(TAG,"Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(err));
        } else if (err != ESP_OK) {
                ESP_LOGI(TAG,"Internal error: %s\n", esp_err_to_name(err));
        }
        /* linenoise allocates line buffer on the heap, so need to free it */
        linenoiseFree(line);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
