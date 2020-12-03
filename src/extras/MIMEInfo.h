#ifndef MIMEInfo_H
#define MIMEInfo_H
#include <Arduino.h>

enum FileExtension
{
  html,
  htm,
  css,
  txt,
  js,
  json,
  png,
  gif,
  jpg,
  ico,
  svg,
  ttf,
  otf,
  woff,
  woff2,
  eot,
  sfnt,
  xml,
  pdf,
  zip,
  gz,
  appcache,
  none,
  maxType
};

struct MimeProp
{
   char endsWith[10];
   char mimeType[50];
};


const MimeProp MIMEInfo[maxType] PROGMEM = 
{
    {".html", "text/html"},
    {".htm", "text/html"},
    {".css", "text/css"},
    {".txt", "text/plain"},
    {".js", "application/javascript"},
    {".json", "application/json"},
    {".png", "image/png"},
    {".gif", "image/gif"},
    {".jpg", "image/jpeg"},
    {".ico", "image/x-icon"},
    {".svg", "image/svg+xml"},
    {".ttf", "application/x-font-ttf"},
    {".otf", "application/x-font-opentype"},
    {".woff", "application/font-woff"},
    {".woff2", "application/font-woff2"},
    {".eot", "application/vnd.ms-fontobject"},
    {".sfnt", "application/font-sfnt"},
    {".xml", "text/xml"},
    {".pdf", "application/pdf"},
    {".zip", "application/zip"},
    {".gz", "application/x-gzip"},
    {".appcache", "text/cache-manifest"},
    {"", "application/octet-stream"}    
};

#endif
