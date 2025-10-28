# BeaverAlarm CCTV Integration Guide

BeaverAlarm now exposes an ONVIF-ready PTZ control surface and embeds an HLS video feed for the "BeaverAlarm CCTV PTZ IP CAM" source.
This document explains how to configure the camera credentials securely, run the HLS transcoding pipeline, and exercise the PTZ REST API.

## 1. Streaming technology

* **Protocol:** HTTP Live Streaming (HLS)
* **Rationale:** HLS is well supported by browsers, plays nicely with caching/CDNs, and can be produced directly from the RTSP
  feed using the FFmpeg toolchain (which relies on `libavformat`). The BeaverAlarm frontend loads the playlist URL exposed by the
  backend configuration and falls back to `hls.js` if the browser lacks native HLS playback.

## 2. Connection details

BeaverAlarm now ships with baked-in CCTV settings so the demo camera works out of the box. The defaults live in
`src/core/cctv_config.cpp` and resolve to the following values:

| Setting | Value |
| --- | --- |
| RTSP URI | `rtsp://admin:MC44rg99qc%40@192.168.1.47:554/cam/realmonitor?channel=1&subtype=1` |
| Camera host | `192.168.1.47` |
| PTZ credentials | `admin` / `MC44rg99qc@` |
| ONVIF path | `onvif/ptz_service` |
| ONVIF profile token | `Profile_1` |
| HLS playlist URL | `http://localhost:8080/streams/beaveralarm/index.m3u8` |

Adjust those constants directly in `load_cctv_config_from_env()` if your network uses a different address or credentials.

## 3. Producing the HLS stream (FFmpeg / libavformat)

The following FFmpeg command (powered by `libavformat`/`libavcodec`) pulls the RTSP feed and emits an HLS playlist that BeaverAlarm
consumes:

```bash
ffmpeg -rtsp_transport tcp \
  -i "rtsp://admin:MC44rg99qc%40@192.168.1.47:554/cam/realmonitor?channel=1&subtype=1" \
  -c:v copy -c:a aac -f hls \
  -hls_time 2 -hls_list_size 6 -hls_flags delete_segments+program_date_time \
  /var/www/html/streams/beaveralarm/index.m3u8
```

If your RTSP feed is public, you can remove the credentials segment entirely and point FFmpeg at the
direct URI reported by the camera, for example:

```bash
ffmpeg -rtsp_transport tcp \
  -i "rtsp://[192.168.1.47]/cam/realmonitor?channel=1&subtype=1" \
  -c:v copy -c:a aac -f hls \
  -hls_time 2 -hls_list_size 6 -hls_flags delete_segments+program_date_time \
  /var/www/html/streams/beaveralarm/index.m3u8
```

Serve the resulting directory (`/var/www/html/streams/beaveralarm/`) over HTTPS or behind an authenticated gateway so that
the configured playlist URL (`http://localhost:8080/streams/beaveralarm/index.m3u8` by default) points to a reachable resource
for the kiosk.

## 4. PTZ REST API

The HTTP backend exposes ONVIF PTZ helpers under `/api/ptz`:

| Action | Request |
| --- | --- |
| Pan left | `POST /api/ptz?action=left` |
| Pan right | `POST /api/ptz?action=right` |
| Tilt up | `POST /api/ptz?action=up` |
| Tilt down | `POST /api/ptz?action=down` |
| Zoom in | `POST /api/ptz?action=zoom_in` |
| Zoom out | `POST /api/ptz?action=zoom_out` |
| Stop motion | `POST /api/ptz?action=stop` |

Every request returns JSON indicating whether the ONVIF SOAP call succeeded. When `libcurl` is unavailable at runtime the
controller logs the prepared SOAP payload but reports an error so operators can spot missing dependencies.

## 5. Live stream metadata endpoint

`GET /api/cctv/stream` returns the configured protocol, playlist URL, and RTSP URI preview so auxiliary dashboards can verify the
camera wiring without exposing secrets in the HTML markup.

## 6. Frontend behaviour

* The BeaverAlarm dashboard auto-loads the HLS playlist when configuration is valid and toggles status labels between
  "Stream ready" and "Stream offline".
* PTZ control buttons issue REST calls and show a busy state while the request is in flight.
* When configuration is incomplete, the UI displays guidance pulled from the translation catalog instead of exposing
  partial credentials.

## 7. Troubleshooting checklist

1. Confirm the values baked into `src/core/cctv_config.cpp` match your camera's host, credentials, and playlist URL.
2. Ensure the FFmpeg process is reachable from the kiosk (playlist served over HTTP/HTTPS).
3. Verify ONVIF credentials by cURLing the PTZ endpoint manually (e.g. `curl -u admin:change-me http://<camera>/onvif/ptz_service`).
4. Watch the kiosk logs for messages starting with `PTZ` to confirm SOAP commands are acknowledged by the camera.

