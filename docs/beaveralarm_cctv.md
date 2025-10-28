# BeaverAlarm CCTV Integration Guide

BeaverAlarm now exposes an ONVIF-ready PTZ control surface and embeds an HLS video feed for the "BeaverAlarm CCTV PTZ IP CAM" source.
This document explains how to configure the camera credentials securely, run the HLS transcoding pipeline, and exercise the PTZ REST API.

## 1. Streaming technology

* **Protocol:** HTTP Live Streaming (HLS)
* **Rationale:** HLS is well supported by browsers, plays nicely with caching/CDNs, and can be produced directly from the RTSP
  feed using the FFmpeg toolchain (which relies on `libavformat`). The BeaverAlarm frontend loads the playlist URL exposed by the
  backend configuration and falls back to `hls.js` if the browser lacks native HLS playback.

## 2. Environment variables

Credentials and network information stay outside of the repository. Copy `.env.example` to `.env` and update the values:

```bash
cp .env.example .env
```

| Variable | Description |
| --- | --- |
| `BEAVER_ALARM_CCTV_HOST` | IP or hostname of the camera (e.g. `192.168.1.47`). |
| `BEAVER_ALARM_CCTV_RTSP_PATH` | RTSP path segment reported by the camera (`cam/realmonitor?channel=1&subtype=1`). |
| `BEAVER_ALARM_CCTV_USERNAME` / `BEAVER_ALARM_CCTV_PASSWORD` | Credentials for both RTSP and ONVIF PTZ calls. |
| `BEAVER_ALARM_ONVIF_PATH` | ONVIF PTZ control path (defaults to `onvif/ptz_service`). |
| `BEAVER_ALARM_ONVIF_PROFILE` | Profile token to target when sending PTZ commands (`Profile_1`, etc.). |
| `BEAVER_ALARM_HLS_URL` | Publicly reachable URL of the generated `.m3u8` playlist. |

> **Note:** `.env` is git-ignored to prevent accidental credential leaks.

## 3. Producing the HLS stream (FFmpeg / libavformat)

The following FFmpeg command (powered by `libavformat`/`libavcodec`) pulls the RTSP feed and emits an HLS playlist that BeaverAlarm
consumes:

```bash
ffmpeg -rtsp_transport tcp \
  -i "rtsp://${BEAVER_ALARM_CCTV_USERNAME}:${BEAVER_ALARM_CCTV_PASSWORD}@${BEAVER_ALARM_CCTV_HOST}/${BEAVER_ALARM_CCTV_RTSP_PATH}" \
  -c:v copy -c:a aac -f hls \
  -hls_time 2 -hls_list_size 6 -hls_flags delete_segments+program_date_time \
  /var/www/html/streams/beaveralarm/index.m3u8
```

Serve the resulting directory (`/var/www/html/streams/beaveralarm/`) over HTTPS or behind an authenticated gateway so that
`BEAVER_ALARM_HLS_URL` points to a reachable playlist, e.g. `https://kiosk.local/streams/beaveralarm/index.m3u8`.

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

1. Confirm `.env` contains the latest host, username, password, and HLS URL.
2. Ensure the FFmpeg process is reachable from the kiosk (playlist served over HTTP/HTTPS).
3. Verify ONVIF credentials by cURLing the PTZ endpoint manually (e.g. `curl -u admin:change-me http://<camera>/onvif/ptz_service`).
4. Watch the kiosk logs for messages starting with `PTZ` to confirm SOAP commands are acknowledged by the camera.

