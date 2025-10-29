# BeaverAlarm USB Webcam Notes

BeaverAlarm now assumes that live video is handled by Debian's built-in webcam tooling instead of a bespoke CCTV pipeline. The kiosk shell focuses on keypad and status experiences while operators launch a separate viewer when a feed is required.

## 1. Direct capture commands

Use standard V4L2 sources to open a USB webcam without proxy servers or MJPEG gateways:

```bash
gst-launch-1.0 v4l2src device=/dev/video0 ! videoconvert ! autovideosink
```

```bash
ffmpeg -f v4l2 -i /dev/video0 -f sdl "USB Camera"
```

Both commands stream pixels straight from the kernel to the GPU with no intermediate network protocol, so the old `feed.mjpeg` endpoint and “connection refused” errors disappear.

## 2. Embedding in custom GTK utilities

The same pipeline can be embedded inside a C++/GTK helper if you need an on-device monitor:

```cpp
GstElement* pipeline = gst_parse_launch(
    "v4l2src device=/dev/video0 ! videoconvert ! autovideosink",
    nullptr);
gst_element_set_state(pipeline, GST_STATE_PLAYING);
```

Keep the pipeline scoped to the helper process; the kiosk UI no longer provisions overlays, PTZ controls, or environment variables for camera setup.

## 3. Operational guidance

* Treat the webcam session as an operator tool—launch it when maintenance staff need a live view and close it when they are done.
* If multiple devices are attached, enumerate them under `/dev/video*` and point the pipeline at the desired node.
* Packaging this helper as a `.desktop` shortcut keeps the workflow consistent with the rest of the BeaverAlarm console.

By leaning on Debian’s webcam stack we avoid maintaining ONVIF credentials, RTSP conversion jobs, and libcurl SOAP calls while still delivering live video when it is genuinely needed.
