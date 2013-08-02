// Spitfire headers
#include <spitfire/storage/filesystem.h>

// Diesel headers
#include "imageloadthread.h"
#include "util.h"

namespace diesel
{
  // ** cPhoto

  class cPhoto
  {
  public:
    cPhoto();

    string_t sFilePath;

    // NOTE: The camera may have created a raw, dng, image, or a combination of these.  The user may also have converted to dng or exported an image
    bool bHasRaw; // Nef, crw, etc.
    bool bHasDNG;
    bool bHasImage; // Jpg, png, etc.
  };

  inline cPhoto::cPhoto() :
    bHasRaw(false),
    bHasDNG(false),
    bHasImage(false)
  {
  }


  // ** cFolderLoadRequest

  cFolderLoadRequest::cFolderLoadRequest(const string_t& _sFolderPath, IMAGE_SIZE _imageSize) :
    sFolderPath(_sFolderPath),
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

  void cImageLoadThread::LoadFolder(const string_t& sFolderPath, IMAGE_SIZE imageSize)
  {
    // If we are adding a folder request then we can reset our loading process interface
    loadingProcessInterface.Reset();

    // Add an event to the queue
    requestQueue.AddItemToBack(new cFolderLoadRequest(sFolderPath, imageSize));
  }

  void cImageLoadThread::StopLoading()
  {
    loadingProcessInterface.SetStop();

    // Remove all the remaining events
    ClearEventQueue();
  }

  void cImageLoadThread::ClearEventQueue()
  {
    // Remove and delete all events on the queue
    while (true) {
      cFolderLoadRequest* pEvent = requestQueue.RemoveItemFromFront();
      if (pEvent == nullptr) break;

      spitfire::SAFE_DELETE(pEvent);
    }
  }

  void cImageLoadThread::ThreadFunction()
  {
    LOG<<"cImageLoadThread::ThreadFunction"<<std::endl;

    while (true) {
      //LOG<<"cImageLoadThread::ThreadFunction Loop"<<std::endl;
      soAction.WaitTimeoutMS(1000);

      if (IsToStop()) break;

      //LOG<<"cImageLoadThread::ThreadFunction Loop getting event"<<std::endl;
      cFolderLoadRequest* pRequest = requestQueue.RemoveItemFromFront();
      if (pRequest != nullptr) {

        // Collect a list of the files in this directory
        std::list<string_t> folders;
        std::map<string_t, cPhoto*> files;

        for (spitfire::filesystem::cFolderIterator iter(pRequest->sFolderPath); iter.IsValid(); iter.Next()) {
          const string_t sFilePath = iter.GetFullPath();
          const string_t sFileNameNoExtension = spitfire::filesystem::GetFileNoExtension(iter.GetFileOrFolder());

          if (iter.IsFolder()) {
            // Tell the handler that we found a folder
            handler.OnFolderFound(sFileNameNoExtension);

            folders.push_back(sFileNameNoExtension);
            continue;
          }

          const string_t sExtension = spitfire::filesystem::GetExtension(iter.GetFileOrFolder());
          const string_t sExtensionLower = spitfire::string::ToLower(sExtension);
          if (!util::IsFileTypeSupported(sExtensionLower)) continue;

          // Change the extension of all supported files to lower case
          if (sExtensionLower != sExtension) {
            const string_t sFrom = spitfire::filesystem::MakeFilePath(pRequest->sFolderPath, sFileNameNoExtension + sExtension);
            const string_t sTo = spitfire::filesystem::MakeFilePath(pRequest->sFolderPath, sFileNameNoExtension + sExtensionLower);
            LOG<<"cImageLoadThread::ThreadFunction Moving file from \""<<sFrom<<"\" to\""<<sTo<<"\""<<std::endl;
            spitfire::filesystem::MoveFile(sFrom, sTo);
          }

          cPhoto* pPhoto = nullptr;

          std::map<string_t, cPhoto*>::iterator found = files.find(sFileNameNoExtension);
          if (found != files.end()) pPhoto = found->second;
          else {
            // Add a new photo
            pPhoto = new cPhoto;
            files[sFileNameNoExtension] = pPhoto;
            pPhoto->sFilePath = sFilePath;
          }

          if (util::IsFileTypeRaw(sExtensionLower)) pPhoto->bHasRaw = true;
          else if (sExtensionLower == TEXT(".dng")) pPhoto->bHasDNG = true;
          else if (util::IsFileTypeImage(sExtensionLower)) pPhoto->bHasImage = true;
        }


        {
          // Tell the handler about the files that were found
          std::map<string_t, cPhoto*>::const_iterator iter = files.begin();
          const std::map<string_t, cPhoto*>::const_iterator iterEnd = files.end();
          while (iter != iterEnd) {
            handler.OnFileFound(iter->first);

            iter++;
          }
        }


        // Load the images
        std::map<string_t, cPhoto*>::iterator iter = files.begin();
        const std::map<string_t, cPhoto*>::iterator iterEnd = files.end();
        while (iter != iterEnd) {
          if (IsToStop() || loadingProcessInterface.IsToStop()) break;

          const string_t sFileNameNoExtension = iter->first;

          // Convert from raw to dng
          cPhoto* pPhoto = iter->second;
          if (pPhoto->bHasRaw && !pPhoto->bHasDNG) {
            const string_t sExtension = util::FindFileExtensionForRawFile(pRequest->sFolderPath, sFileNameNoExtension);
            ASSERT(!sExtension.empty());

            const string_t sFilePathRAW = spitfire::filesystem::MakeFilePath(pRequest->sFolderPath, sFileNameNoExtension + sExtension);

            // Convert the file to dng and use the dng
            const string_t sFilePathDNG = imageCacheManager.GetOrCreateDNGForRawFile(sFilePathRAW);
            if (sFilePathDNG.empty()) {
              // There was an error converting to dng so we need to notify the handler
              handler.OnImageError(sFileNameNoExtension, pRequest->imageSize);

              iter++;

              continue;
            }

            pPhoto->bHasDNG = true;
          }

          string_t sThumbnailFilePath;

          if (pPhoto->bHasDNG) {
            const string_t sFilePathDNG = spitfire::filesystem::MakeFilePath(pRequest->sFolderPath, sFileNameNoExtension + TEXT(".dng"));

            // Create thumbnail from dng
            sThumbnailFilePath = imageCacheManager.GetOrCreateThumbnailForDNGFile(sFilePathDNG, pRequest->imageSize);

          } else {
            ASSERT(pPhoto->bHasImage);
            const string_t sExtension = util::FindFileExtensionForImageFile(pRequest->sFolderPath, sFileNameNoExtension);
            ASSERT(!sExtension.empty());
            const string_t sFilePathImage = spitfire::filesystem::MakeFilePath(pRequest->sFolderPath, sFileNameNoExtension + sExtension);

            // Create thumbnail from image
            sThumbnailFilePath = imageCacheManager.GetOrCreateThumbnailForImageFile(sFilePathImage, pRequest->imageSize);
          }

          // Loading the image can take a while so we need to check again if we should stop
          if (IsToStop() || loadingProcessInterface.IsToStop()) break;

          // Load the thumbnail image
          voodoo::cImage* pImage = new voodoo::cImage;

          ASSERT(!sThumbnailFilePath.empty());
          pImage->LoadFromFile(sThumbnailFilePath);

          // Notify the handler
          if (pImage->IsValid()) handler.OnImageLoaded(sFileNameNoExtension, pRequest->imageSize, pImage);
          else {
            handler.OnImageError(sFileNameNoExtension, pRequest->imageSize);

            // Delete the image
            delete pImage;
          }

          iter++;
        }

        {
          // Delete the photos
          std::map<string_t, cPhoto*>::iterator iter = files.begin();
          const std::map<string_t, cPhoto*>::iterator iterEnd = files.end();
          while (iter != iterEnd) {
            spitfire::SAFE_DELETE(iter->second);

            iter++;
          }
        }

        //LOG<<"cImageLoadThread::ThreadFunction Loop deleting event"<<std::endl;
        spitfire::SAFE_DELETE(pRequest);
      } else {
        // If the queue is empty then we know that there are no more actions and it is safe to reset our stop loading signal object
        loadingProcessInterface.Reset();
      }

      // Try to avoid hogging the CPU
      spitfire::util::SleepThisThreadMS(1);
      spitfire::util::YieldThisThread();
    }

    // Remove any further events because we don't care any more
    ClearEventQueue();

    LOG<<"cImageLoadThread::ThreadFunction returning"<<std::endl;
  }
}
