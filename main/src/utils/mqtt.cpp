#include "utils/mqtt.h"
#define TAG "MQTT"
bool mqttConnected = false;

void subscribeToTopics(esp_mqtt_client_handle_t client) {
    ESP_LOGI(TAG, "Subscribing to topics: %s", MQTT_DEVICE_TOPIC);
    esp_mqtt_client_subscribe(client, (const char*)MQTT_DEVICE_TOPIC, 0);
    ESP_LOGI(TAG, "Subscribing to topics: %s", "electric/price");
    esp_mqtt_client_subscribe(client, "electric/price", 0);
}

esp_mqtt_client_handle_t client;
//=======================================================================
//======================== MQTT testing =================================
//=======================================================================
static void log_error_if_nonzero(const char* message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}
/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = static_cast<esp_mqtt_event_handle_t>(event_data);
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    mqttMessage mqtt;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        subscribeToTopics(client);
        xEventGroupSetBits(mqttEventGroup, MQTT_CONNECTED);
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        mqttConnected = false;
        xEventGroupSetBits(mqttEventGroup, MQTT_DISCONNECTED);
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_DATA:
        xEventGroupSetBits(mqttEventGroup, MQTT_NEW_MESSAGE);
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        ESP_LOGI(TAG, "TOPIC=%.*s\r\n", event->topic_len, event->topic);
        ESP_LOGI(TAG, "DATA=%.*s\r\n", event->data_len, event->data);

        sprintf(mqtt.message, "%.*s", event->data_len, event->data);
        sprintf(mqtt.topic, "%.*s", event->topic_len, event->topic);
        xQueueSend(mqttQueue, &mqtt, 0);
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno", event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

bool mqtt_app_start(void) {

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_BASE", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("outbox", ESP_LOG_VERBOSE);

    esp_mqtt_client_config_t mqtt_cfg = {};
    char mqttAddress[128]; // Adjust the size according to your URI length

    snprintf(mqttAddress, sizeof(mqttAddress), "mqtt://%s:%s", MQTT_BROKER_URL, MQTT_BROKER_PORT);
    mqtt_cfg.broker.address.uri = mqttAddress;

    // MQTT buffer configuration
    esp_mqtt_client_config_t::buffer_t buffer_config = {}; // Create an instance of buffer_t struct
    buffer_config.size = 2048; // Set the members of the buffer_t struct
    buffer_config.out_size = 2048;
    mqtt_cfg.buffer = buffer_config; // Assign the buffer_t struct instance to the buffer_t member of the mqtt_cf

    // MQTT task configuration
    esp_mqtt_client_config_t::task_t taskConfig = {};
    taskConfig.stack_size = 4096;
    taskConfig.priority = 5;
    mqtt_cfg.task = taskConfig;

    // MQTT client configuration
    esp_mqtt_client_config_t::credentials_t credentials = {};
    credentials.username = DEVICE_ID;
    esp_mqtt_client_config_t::credentials_t::authentication_t auth = {};
    auth.password = MQTT_PASSWORD;

    credentials.authentication = auth;
    mqtt_cfg.credentials = credentials;

    //client->config->path
    ESP_LOGI(TAG, "Broker url: %s", mqtt_cfg.broker.address.uri);
    client = esp_mqtt_client_init(&mqtt_cfg);
    if (client == NULL) {
        ESP_LOGE("MQTT", "Failed to initialize client");
    }
    /* The last argument may be used to pass data to the event handler, in this
     * example mqtt_event_handler */
    esp_mqtt_client_register_event(client, MQTT_EVENT_ANY, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);

    while (!mqttConnected) {
        // Send the message to notify server that a new device is connected
        esp_mqtt_client_publish(client, (const char*)CONFIG_MQTT_NEW_DEVICE_TOPIC, DEVICE_ID, 0, 1, 0);
        EventBits_t bits = xEventGroupWaitBits(mqttEventGroup, MQTT_NEW_MESSAGE, pdFALSE, pdFALSE, 3000 / portTICK_PERIOD_MS);
        if (bits & MQTT_NEW_MESSAGE) {
            ESP_LOGI(TAG, "First MQTT message received, connected to broker");
            xEventGroupClearBits(mqttEventGroup, MQTT_NEW_MESSAGE);
            mqttConnected = true;
        }
    }
    ESP_LOGI(TAG, "RETURNING FROM MQTT START");
    return true;
}