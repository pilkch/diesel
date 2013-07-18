// Spitfire headers
#include <spitfire/storage/filesystem.h>

// Diesel headers
#include "imageloadthread.h"
#include "util.h"

namespace diesel
{
  cImageLoadRequest::cImageLoadRequest(const string_t& _sFilePath, IMAGE_SIZE _imageSize) :
    sFilePath(_sFilePath),
    imageSize(_imageSize)
  {
  }


  // ** cImageLoadThread

  cImageLoadThread::cImageLoadThread(cImageLoadHandler& _handler) :
    spitfire::util::cThread(soAction, "cImageLoadThread::cThread"),
    handler(_handler),
    soAction("cImageLoadThread::soAction"),
    requestQueue(soAction)
  {
  }

  void cImageLoadThread::Start()
  {
    // Start
    Run();
  }

  void cImageLoadThread::StopSoon()
  {
    StopThreadSoon();
  }

  void cImageLoadThread::StopNow()
  {
    StopThreadNow();
  }

  void cImageLoadThread::LoadThumbnail(const string_t& sFilePath, IMAGE_SIZE imageSize)
  {
    // Add an event to the queue
    requestQueue.AddItemToBack(new cImageLoadRequest(sFilePath, imageSize));
  }

  void cImageLoadThread::ThreadFunction()
  {
    LOG<<"cImageLoadThread::ThreadFunction"<<std::endl;

    while (true) {
      //LOG<<"cImageLoadThread::ThreadFunction Loop"<<std::endl;
      soAction.WaitTimeoutMS(1000);

      if (IsToStop()) break;

      //LOG<<"cImageLoadThread::ThreadFunction Loop getting event"<<std::endl;
      cImageLoadRequest* pRequest = requestQueue.RemoveItemFromFront();
      if (pRequest != nullptr) {
        switch (pRequest->imageSize) {
          case IMAGE_SIZE::FULL: {
            if (spitfire::filesystem::FileExists(pRequest->sFilePath)) {
              voodoo::cImage* pImage = new voodoo::cImage;

              pImage->LoadFromFile(pRequest->sFilePath);

              // Notify the handler
              if (pImage->IsValid()) handler.OnImageLoaded(pRequest->sFilePath, pRequest->imageSize, pImage);
              else {
                handler.OnImageError(pRequest->sFilePath, pRequest->imageSize);

                // Delete the image
                delete pImage;
              }
            }
            break;
          }
          default: {
            LOG<<"cImageLoadThread::ThreadFunction Unsupported image size"<<std::endl;
            ASSERT(false);
            break;
          }
        };

        //LOG<<"cImageLoadThread::ThreadFunction Loop deleting event"<<std::endl;
        spitfire::SAFE_DELETE(pRequest);
      }

      // Try to avoid hogging the CPU
      spitfire::util::SleepThisThreadMS(1);
    }

    // Remove any further events because we don't care any more
    while (true) {
      cImageLoadRequest* pRequest = requestQueue.RemoveItemFromFront();
      if (pRequest == nullptr) break;

      spitfire::SAFE_DELETE(pRequest);
    }

    LOG<<"cImageLoadThread::ThreadFunction returning"<<std::endl;
  }
}
