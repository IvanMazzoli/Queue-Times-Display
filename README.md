# Queue-Times-Display
![stop_display](https://github.com/user-attachments/assets/6b612c79-3ea5-499f-b9f1-82579d42adf4)

An ESP8266 and SSD1306 device to display your favourite attraction waiting time.

## Required Hardware
- 1x ESP8266 devboard (D1 Mini recommended) [Buy on AliExpress](https://it.aliexpress.com/item/1005006235653025.html)
- 1x SSD1306 0.96" OLED I2C display [Buy On AliExpress](https://it.aliexpress.com/item/1005006085392157.html)
- 1x 3D printed case (optional, but recommended)

## Serverless Version (Queue_Times_Display.ino)
This release was created for low specs devices with less on board memory available.
To use this version you need to set the following information in the script:
- Your WiFi network name and password (**lines 17, 18**);
- The correct URL for the park queues (**line 176**);
- The JSON path of your ride (**line 216**).

To obtain the waiting data it is necessary to extract the JSON path from the Queue-Times.com API at the following address:
- For the park: https://queue-times.com/parks.json
- For the queues: https://queue-times.com/parks/PARK_ID/queue_times.json

Let's take Phantasialand's "**Taron**" attraction for example. First of all we need to obtain the park ID from the first link, in the case of Phantasialand it is **56**. We then replace it within the second link to obtain the link with the waiting times in the park:

- https://queue-times.com/parks/56/queue_times.json

We must now recover the JSON path for Taron, that is:

`JsonObject path = doc["lands"][5]["rides"][3];`

Let's analyze the path to understand how to adapt it to another park. The first element is the root of the JSON document, "**lands**", which is an array of all the various areas of the selected park. From this array we must then select the land that contains Taron, **Mystery**, with index **5**. We therefore obtain from the "**rides**" array the index of the Taron attraction, in our case **3**.

## Webserver Version (coming soon)
This version, still in development, integrates a web server that allows you to select the network to connect to and the attraction to monitor without having to connect the device to a PC and use the Arduino IDE to update the data.

## Sources
- [Queue-Times.com](https://queue-times.com/) for their core API for this project;
- [Wemos D1 Mini OLED Wedge (Updated Design)](https://www.thingiverse.com/thing:4848322) by zeustopher on Thinkiverse for the display case.