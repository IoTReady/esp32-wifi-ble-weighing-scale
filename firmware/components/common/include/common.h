//
//  common.h
//  Weighing_Scale_Firmware
//
//  ESP32 firmware for Smart Weighing Scale
//
//  This module provides functions and queues to pass data between other modules.
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

#ifndef __COMMON_H__
#define __COMMON_H__ 1

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "wifi_sta.h" 

// Used by the OTA module to check if the current version is different from the version
// on the server, i.e. if an upgrade or downgrade should be performed.
#define SOFTWARE_VERSION          0

// Provide the network name and password of your WIFI network.
#define DEFAULT_WIFI_SSID         "myssid"
#define DEFAULT_WIFI_PASSWORD     "mypassword"
#define DEFAULT_TIMEOUT 10 //ticks = 100ms

// Default user preferences
#define DEFAULT_LIGHT_MODE 2 // combo
#define DEFAULT_SESSION_TIME 10 // minutes
#define DEFAULT_PAUSE_LIMIT 30 // minutes
#define DEFAULT_COOLING_PERIOD 5 // minutes
#define MAX_SYSTEM_TEMPERATURE 50 // celcius

#define DEFAULT_BLE_NAME "IOTReadyScale"

// Priorities
#define LOW_PRIORITY 5
#define MEDIUM_PRIORITY 10


// Stack sizes

#define LARGEST_STACK 15360
#define LARGER_STACK 8192
#define LARGE_STACK 4096
#define MEDIUM_STACK 2048
#define SMALL_STACK 1024

#define LINK_MODE_FOLLOW 1
#define LINK_MODE_LEAD 2
#define LINK_MODE_NEUTRAL 3

// Provide server name, path to metadata file and polling interval for OTA updates.
#define OTA_SERVER_HOST_NAME      "galtofkgp.github.io"
#define OTA_SERVER_METADATA_PATH  "/esp32/ota.txt"
#define OTA_POLLING_INTERVAL_S    60
#define OTA_AUTO_REBOOT           1

// Provide the Root CA certificate for chain validation.
// (copied from browser certificates for https://galtofkgp.github.io)
#define OTA_SERVER_ROOT_CA_PEM \
    "-----BEGIN CERTIFICATE-----\n" \
    "MIIDxTCCAq2gAwIBAgIQAqxcJmoLQJuPC3nyrkYldzANBgkqhkiG9w0BAQUFADBs\n" \
    "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n" \
    "d3cuZGlnaWNlcnQuY29tMSswKQYDVQQDEyJEaWdpQ2VydCBIaWdoIEFzc3VyYW5j\n" \
    "ZSBFViBSb290IENBMB4XDTA2MTExMDAwMDAwMFoXDTMxMTExMDAwMDAwMFowbDEL\n" \
    "MAkGA1UEBhMCVVMxFTATBgNVBAoTDERpZ2lDZXJ0IEluYzEZMBcGA1UECxMQd3d3\n" \
    "LmRpZ2ljZXJ0LmNvbTErMCkGA1UEAxMiRGlnaUNlcnQgSGlnaCBBc3N1cmFuY2Ug\n" \
    "RVYgUm9vdCBDQTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMbM5XPm\n" \
    "+9S75S0tMqbf5YE/yc0lSbZxKsPVlDRnogocsF9ppkCxxLeyj9CYpKlBWTrT3JTW\n" \
    "PNt0OKRKzE0lgvdKpVMSOO7zSW1xkX5jtqumX8OkhPhPYlG++MXs2ziS4wblCJEM\n" \
    "xChBVfvLWokVfnHoNb9Ncgk9vjo4UFt3MRuNs8ckRZqnrG0AFFoEt7oT61EKmEFB\n" \
    "Ik5lYYeBQVCmeVyJ3hlKV9Uu5l0cUyx+mM0aBhakaHPQNAQTXKFx01p8VdteZOE3\n" \
    "hzBWBOURtCmAEvF5OYiiAhF8J2a3iLd48soKqDirCmTCv2ZdlYTBoSUeh10aUAsg\n" \
    "EsxBu24LUTi4S8sCAwEAAaNjMGEwDgYDVR0PAQH/BAQDAgGGMA8GA1UdEwEB/wQF\n" \
    "MAMBAf8wHQYDVR0OBBYEFLE+w2kD+L9HAdSYJhoIAu9jZCvDMB8GA1UdIwQYMBaA\n" \
    "FLE+w2kD+L9HAdSYJhoIAu9jZCvDMA0GCSqGSIb3DQEBBQUAA4IBAQAcGgaX3Nec\n" \
    "nzyIZgYIVyHbIUf4KmeqvxgydkAQV8GK83rZEWWONfqe/EW1ntlMMUu4kehDLI6z\n" \
    "eM7b41N5cdblIZQB2lWHmiRk9opmzN6cN82oNLFpmyPInngiK3BD41VHMWEZ71jF\n" \
    "hS9OMPagMRYjyOfiZRYzy78aG6A9+MpeizGLYAiJLQwGXFK3xPkKmNEVX58Svnw2\n" \
    "Yzi9RKR/5CYrCsSXaQ3pjOLAEFe4yHYSkVXySGnYvCoCWw9E1CAx2/S6cCZdkGCe\n" \
    "vEsXCS+0yx5DaMkHJ8HSXPfqIbloEpw8nL+e/IBcm2PN7EeqJSdnoDfzAIJ9VNep\n" \
    "+OkuE6N36B9K\n" \
    "-----END CERTIFICATE-----\n"

// Provide the Peer certificate for certificate pinning.
// (copied from browser certificates for https://galtofkgp.github.io)
#define OTA_PEER_PEM \
    "-----BEGIN CERTIFICATE-----\n" \
    "MIIHMTCCBhmgAwIBAgIQDf56dauo4GsS0tOc8/ik/DANBgkqhkiG9w0BAQsFADBw\n" \
    "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n" \
    "d3cuZGlnaWNlcnQuY29tMS8wLQYDVQQDEyZEaWdpQ2VydCBTSEEyIEhpZ2ggQXNz\n" \
    "dXJhbmNlIFNlcnZlciBDQTAeFw0xODA2MjcwMDAwMDBaFw0yMDA2MjAxMjAwMDBa\n" \
    "MGoxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpDYWxpZm9ybmlhMRYwFAYDVQQHEw1T\n" \
    "YW4gRnJhbmNpc2NvMRUwEwYDVQQKEwxHaXRIdWIsIEluYy4xFzAVBgNVBAMTDnd3\n" \
    "dy5naXRodWIuY29tMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAxjbK\n" \
    "Rw+cVNjgA/BhLfRTOAk6jRGT7WRmmWy/qhFmTxBKapW/Kh4UNeD7oUAB0yG+Fyvu\n" \
    "1grVYR5RFUGBmA5mkqrBjCg4fNbFU9vLKIbkhpH3Jp2HVNLzvjzs/RqCN04MEc6M\n" \
    "nbmkjx1YeW9CveXojrt4x3Sa3nsPx6KcQtJpm8MuS8OhV2CJUF1etJpu9id5WJQd\n" \
    "U3j3h65PmXMBvunm7UaGDt2WVK4CRcIb6j0X3H4cjh4WhXLyMcSPutXZVoD0FSmY\n" \
    "+mjXVZCTJd+vHsYX1fj+kge20DJx0F8qmqJEr7bx1lfj7/U4aTgdZBDCB8wIl0fK\n" \
    "cOdR3wq2RrDVRUlOuQIDAQABo4IDyzCCA8cwHwYDVR0jBBgwFoAUUWj/kK8CB3U8\n" \
    "zNllZGKiErhZcjswHQYDVR0OBBYEFI+I3Dwd7kQJ5DCMBsoC859olPB5MHsGA1Ud\n" \
    "EQR0MHKCDnd3dy5naXRodWIuY29tggsqLmdpdGh1Yi5pb4IXKi5naXRodWJ1c2Vy\n" \
    "Y29udGVudC5jb22CDCouZ2l0aHViLmNvbYIKZ2l0aHViLmNvbYIJZ2l0aHViLmlv\n" \
    "ghVnaXRodWJ1c2VyY29udGVudC5jb20wDgYDVR0PAQH/BAQDAgWgMB0GA1UdJQQW\n" \
    "MBQGCCsGAQUFBwMBBggrBgEFBQcDAjB1BgNVHR8EbjBsMDSgMqAwhi5odHRwOi8v\n" \
    "Y3JsMy5kaWdpY2VydC5jb20vc2hhMi1oYS1zZXJ2ZXItZzYuY3JsMDSgMqAwhi5o\n" \
    "dHRwOi8vY3JsNC5kaWdpY2VydC5jb20vc2hhMi1oYS1zZXJ2ZXItZzYuY3JsMEwG\n" \
    "A1UdIARFMEMwNwYJYIZIAYb9bAEBMCowKAYIKwYBBQUHAgEWHGh0dHBzOi8vd3d3\n" \
    "LmRpZ2ljZXJ0LmNvbS9DUFMwCAYGZ4EMAQICMIGDBggrBgEFBQcBAQR3MHUwJAYI\n" \
    "KwYBBQUHMAGGGGh0dHA6Ly9vY3NwLmRpZ2ljZXJ0LmNvbTBNBggrBgEFBQcwAoZB\n" \
    "aHR0cDovL2NhY2VydHMuZGlnaWNlcnQuY29tL0RpZ2lDZXJ0U0hBMkhpZ2hBc3N1\n" \
    "cmFuY2VTZXJ2ZXJDQS5jcnQwDAYDVR0TAQH/BAIwADCCAX4GCisGAQQB1nkCBAIE\n" \
    "ggFuBIIBagFoAHYA7ku9t3XOYLrhQmkfq+GeZqMPfl+wctiDAMR7iXqo/csAAAFk\n" \
    "QsgYsgAABAMARzBFAiBe00KrPC3L9DUwJ0rYXoQ+Fh70UM3rYNyT9uLyEo1kfAIh\n" \
    "ALtR7krXouwsj4s52P2bWJiS0QKGLvbnBGnZLn4WNScDAHYAh3W/51l8+IxDmV+9\n" \
    "827/Vo1HVjb/SrVgwbTq/16ggw8AAAFkQsgYswAABAMARzBFAiB2wURHjtfhDN6l\n" \
    "3Q9gaYnu9h5xsFjJIOA3qO1dzqNVjQIhANPyeXq61y1gsHH/Iq2aqv43CMjjDQ8p\n" \
    "02baNg3puNoMAHYAu9nfvB+KcbWTlCOXqpJ7RzhXlQqrUugakJZkNo4e0YUAAAFk\n" \
    "QsgX9wAABAMARzBFAiEAhxuoQCyQzEoSK8/f4AXq+d0RZ3550opfcEty3p7gHyAC\n" \
    "IBZ3NXHCNZpH6ow7D/qUZcfXkuMk/aZvR9uHoVvkixbcMA0GCSqGSIb3DQEBCwUA\n" \
    "A4IBAQCjPNhowM6/PVcxHtl18r+92pPzU3JRGnXfz/P077yEgIIgs8I0lPqi9AiW\n" \
    "QYev56+Ar4a39y+666s55ogjIVL+7cboKPgqflEY1POCg64UZNDHHPTnd5ba2OUw\n" \
    "dtk1QtrEtYbUKm0W/1Ne9JRdN2rh5J87NsSPhirmKsP6HA3OhL93LTqB72Iehwj+\n" \
    "qmIrQhkhJiTReWTb5sNrD2gIt8Dhuts/M84a9v7Hrkq/VPYVmARm7Py0GC/jqxpI\n" \
    "NCHBEAFKN2c7iqmx1dVOGJDV3U0TCa0yWbnXsZ9M0652zH1obM3UxCtUMuUZzDb9\n" \
    "aTrrJ67dru040mysq2NLPPq+t+EQ\n" \
    "-----END CERTIFICATE-----\n"

// -------------------------------------------------------------------------------------


wifi_sta_init_struct_t wifi_params;
uint8_t weight_data; 
extern int boot_mode;
bool just_booted;
bool allow_reboot;
extern char device_id[13];
extern char OTA_IP[100];
extern char OTA_PORT[100];
extern char OTA_FILENAME[100];
extern float Totalmass;
extern char TotalmassStr[16];

// TaskHandle_t weighingTask_Handle, tareTask_Handle;


// functions

void set_device_id();
#endif // __COMMON_H__
