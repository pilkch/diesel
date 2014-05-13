// Spitfire headers
#include <spitfire/storage/filesystem.h>
#include <spitfire/util/log.h>

// Diesel headers
#include "imagecachemanager.h"
#include "imageloadthread.h"
#include "util.h"

namespace diesel
{
  // ** cFolderLoadThumbnailsRequest

  cFolderLoadThumbnailsRequest::cFolderLoadThumbnailsRequest(const string_t& _sFolderPath) :
    sFolderPath(_sFolderPath)
  {
  }


  // ** cFileLoadFullHighPriorityRequest

  cFileLoadFullHighPriorityRequest::cFileLoadFullHighPriorityRequest(const string_t& _sFileNameNoExtension) :
    sFileNameNoExtension(_sFileNameNoExtension)
  {
  }


  // ** cImageLoadThread

  cImageLoadThread::cImageLoadThread(cImageLoadHandler& _handler) :
    spitfire::util::cThread(soAction, TEXT("cImageLoadThread::cThread")),
    handler(_handler),
    soAction(TEXT("cImageLoadThread::soAction")),
    requestQueue(soAction),
    highPriorityRequestQueue(soAction),
    mutexMaximumCacheSize(TEXT("cImageLoadThread::mutexMaximumCacheSize"))
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

  void cImageLoadThread::SetMaximumCacheSizeGB(size_t nSizeGB)
  {
    spitfire::util::cLockObject lock(mutexMaximumCacheSize);
    nMaximumCacheSizeGB = nSizeGB;
  }

  void cImageLoadThread::LoadFolderThumbnails(const string_t& sFolderPath)
  {
    // If we are adding a folder request then we can reset our loading process interface
    loadingProcessInterface.Reset();

    // Add an event to the queue
    requestQueue.AddItemToBack(new cFolderLoadThumbnailsRequest(sFolderPath));
  }

  void cImageLoadThread::LoadFileFullHighPriority(const string_t& sFilePath)
  {
    // Add an event to the queue
    highPriorityRequestQueue.AddItemToBack(new cFileLoadFullHighPriorityRequest(sFilePath));
  }

  void cImageLoadThread::StopLoading()
  {
    loadingProcessInterface.SetStop();

    // Remove all the remaining events
    ClearEventQueue();
  }

  void cImageLoadThread::ClearEventQueue()
  {
    // Remove and delete all folder load events on the queue
    while (true) {
      cFolderLoadThumbnailsRequest* pEvent = requestQueue.RemoveItemFromFront();
      if (pEvent == nullptr) break;

      spitfire::SAFE_DELETE(pEvent);
    }

    // Remove and delete all file load events on the queue
    while (true) {
      cFileLoadFullHighPriorityRequest* pEvent = highPriorityRequestQueue.RemoveItemFromFront();
      if (pEvent == nullptr) break;

      spitfire::SAFE_DELETE(pEvent);
    }
  }

  bool cImageLoadThread::GetOrCreateDNGForRawFile(const string_t& sFolderPath, const string_t& sFileNameNoExtension, cPhoto& photo)
  {
    if (spitfire::filesystem::GetLastDirectory(sFolderPath) == TEXT("raw")) {
      LOG<<"cImageLoadThread::GetOrCreateDNGForRawFile Skipping files in raw/ folder"<<std::endl;
      return false;
    } else LOG<<"folder="<<spitfire::filesystem::GetLastDirectory(sFolderPath)<<std::endl;

    const string_t sExtension = util::FindFileExtensionForRawFile(sFolderPath, sFileNameNoExtension);
    ASSERT(!sExtension.empty());

    const string_t sFilePathRAW = spitfire::filesystem::MakeFilePath(sFolderPath, sFileNameNoExtension + sExtension);

    // Convert the file to dng and use the dng
    const string_t sFilePathDNG = cImageCacheManager::GetOrCreateDNGForRawFile(sFilePathRAW);
    if (sFilePathDNG.empty()) {
      // There was an error converting to dng so we need to notify the handler
      handler.OnImageError(sFileNameNoExtension);

      return false;
    }

    // Creating the dng worked so we should move the raw file into the raw/ folder in that directory
    const string_t sRawFolderPath = spitfire::filesystem::MakeFilePath(sFolderPath, TEXT("raw"));
    if (!spitfire::filesystem::DirectoryExists(sRawFolderPath)) spitfire::filesystem::CreateDirectory(sRawFolderPath);

    const string_t sFilePathRAWInRawFolder = spitfire::filesystem::MakeFilePath(sRawFolderPath, sFileNameNoExtension + sExtension);
    spitfire::filesystem::MoveFile(sFilePathRAW, sFilePathRAWInRawFolder);

    photo.bHasDNG = true;

    return true;
  }

  string_t cImageLoadThread::GetOrCreateThumbnail(const string_t& sFolderPath, const string_t& sFileNameNoExtension, IMAGE_SIZE imageSize, cPhoto& photo)
  {
    string_t sThumbnailFilePath;

    if (photo.bHasDNG) {
      const string_t sFilePathDNG = spitfire::filesystem::MakeFilePath(sFolderPath, sFileNameNoExtension + TEXT(".dng"));

      // Create thumbnail from dng
      sThumbnailFilePath = cImageCacheManager::GetOrCreateThumbnailForDNGFile(sFilePathDNG, imageSize);

    } else {
      ASSERT(photo.bHasImage);
      const string_t sExtension = util::FindFileExtensionForImageFile(sFolderPath, sFileNameNoExtension);
      ASSERT(!sExtension.empty());
      const string_t sFilePathImage = spitfire::filesystem::MakeFilePath(sFolderPath, sFileNameNoExtension + sExtension);

      // Create thumbnail from image
      sThumbnailFilePath = cImageCacheManager::GetOrCreateThumbnailForImageFile(sFilePathImage, imageSize);
    }

    return sThumbnailFilePath;
  }

  void cImageLoadThread::LoadThumbnailImage(const string_t& sThumbnailFilePath, const string_t& sFileNameNoExtension, IMAGE_SIZE imageSize)
  {
    ASSERT(!sThumbnailFilePath.empty());

    // Load the thumbnail image
    voodoo::cImage* pImage = new voodoo::cImage;

    pImage->LoadFromFile(sThumbnailFilePath);

    // Notify the handler
    if (pImage->IsValid()) handler.OnImageLoaded(sFileNameNoExtension, imageSize, pImage);
    else {
      handler.OnImageError(sFileNameNoExtension);

      // Delete the image
      delete pImage;
    }
  }

  void cImageLoadThread::HandleHighPriorityRequestQueue(const string_t& sFolderPath, std::map<string_t, cPhoto*>& files)
  {
    while (true) {
      // Loading the image can take a while so we need to check again if we should stop
      if (IsToStop() || loadingProcessInterface.IsToStop()) break;

      //LOG<<"cImageLoadThread::ThreadFunction Loop getting event"<<std::endl;
      cFileLoadFullHighPriorityRequest* pRequest = highPriorityRequestQueue.RemoveItemFromFront();
      if (pRequest == nullptr) break;

      const string_t sFileNameNoExtension = pRequest->sFileNameNoExtension;
      LOG<<"cImageLoadThread::HandleHighPriorityRequestQueue Request found \""<<sFileNameNoExtension<<"\""<<std::endl;

      // Convert from raw to dng
      std::map<string_t, cPhoto*>::iterator iter = files.find(sFileNameNoExtension);
      ASSERT(iter != files.end());
      if (iter != files.end()) {
        LOG<<"cImageLoadThread::HandleHighPriorityRequestQueue Photo found"<<std::endl;
        cPhoto* pPhoto = iter->second;

        bool bStop = false;

        if (pPhoto->bHasRaw && !pPhoto->bHasDNG) {
          LOG<<"cImageLoadThread::HandleHighPriorityRequestQueue Creating DNG"<<std::endl;
          if (!GetOrCreateDNGForRawFile(sFolderPath, sFileNameNoExtension, *pPhoto)) {
            // If the conversion failed then we need to get out of here
            bStop = true;
          }
        }

        // Creating a dng file can take a while so we need to check again if we should stop
        if (IsToStop() || loadingProcessInterface.IsToStop()) bStop = true;

        if (!bStop) {
          LOG<<"cImageLoadThread::HandleHighPriorityRequestQueue Creating thumbnail"<<std::endl;
          const string_t sThumbnailFilePath = GetOrCreateThumbnail(sFolderPath, sFileNameNoExtension, IMAGE_SIZE::FULL, *pPhoto);
          ASSERT(!sThumbnailFilePath.empty());

          LOG<<"cImageLoadThread::HandleHighPriorityRequestQueue Loading thumbnail"<<std::endl;
          LoadThumbnailImage(sThumbnailFilePath, sFileNameNoExtension, IMAGE_SIZE::FULL);
        }
      }

      spitfire::SAFE_DELETE(pRequest);
    }
  }

  void cImageLoadThread::ThreadFunction()
  {
    LOG<<"cImageLoadThread::ThreadFunction"<<std::endl;

    std::list<string_t> folders;
    std::map<string_t, cPhoto*> files;

    string_t sFolderPath;

    while (true) {
      //LOG<<"cImageLoadThread::ThreadFunction Loop"<<std::endl;
      soAction.WaitTimeoutMS(1000);

      if (IsToStop()) break;

      // Check if we need to handle a high priority request
      HandleHighPriorityRequestQueue(sFolderPath, files);

      //LOG<<"cImageLoadThread::ThreadFunction Loop getting event"<<std::endl;
      cFolderLoadThumbnailsRequest* pRequest = requestQueue.RemoveItemFromFront();
      if (pRequest != nullptr) {
        // Remove the known folders
        folders.clear();

        {
          // Delete the photos
          std::map<string_t, cPhoto*>::iterator iter = files.begin();
          const std::map<string_t, cPhoto*>::iterator iterEnd = files.end();
          while (iter != iterEnd) {
            spitfire::SAFE_DELETE(iter->second);

            iter++;
          }

          files.clear();
        }

        // Change our folder
        sFolderPath = pRequest->sFolderPath;

        // Collect a list of the files in this directory
        for (spitfire::filesystem::cFolderIterator iter(sFolderPath); iter.IsValid(); iter.Next()) {
          if (iter.IsFolder()) {
            const string_t sFolderName = iter.GetFileOrFolder();

            // Tell the handler that we found a folder
            handler.OnFolderFound(sFolderName);

            folders.push_back(sFolderName);
            continue;
          }

          const string_t sFilePath = iter.GetFullPath();
          const string_t sFileNameNoExtension = spitfire::filesystem::GetFileNoExtension(iter.GetFileOrFolder());

          const string_t sExtension = spitfire::filesystem::GetExtension(iter.GetFileOrFolder());
          const string_t sExtensionLower = spitfire::string::ToLower(sExtension);
          if (!util::IsFileTypeSupported(sExtensionLower)) continue;

          // Change the extension of all supported files to lower case
          if (sExtensionLower != sExtension) {
            const string_t sFrom = spitfire::filesystem::MakeFilePath(sFolderPath, sFileNameNoExtension + sExtension);
            const string_t sTo = spitfire::filesystem::MakeFilePath(sFolderPath, sFileNameNoExtension + sExtensionLower);
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

          HandleHighPriorityRequestQueue(sFolderPath, files);

          if (IsToStop() || loadingProcessInterface.IsToStop()) break;

          const string_t sFileNameNoExtension = iter->first;

          // Convert from raw to dng
          cPhoto* pPhoto = iter->second;
          ASSERT(pPhoto != nullptr);
          if (pPhoto->bHasRaw && !pPhoto->bHasDNG) {
            if (!GetOrCreateDNGForRawFile(sFolderPath, sFileNameNoExtension, *pPhoto)) {
              // If the conversion failed then we need to skip to the next file
              iter++;
              continue;
            }
          }

          // Creating a dng file can take a while so we need to check again if we should stop
          if (IsToStop() || loadingProcessInterface.IsToStop()) break;

          HandleHighPriorityRequestQueue(sFolderPath, files);

          if (IsToStop() || loadingProcessInterface.IsToStop()) break;

          const string_t sThumbnailFilePath = GetOrCreateThumbnail(sFolderPath, sFileNameNoExtension, IMAGE_SIZE::THUMBNAIL, *pPhoto);

          // Loading the image can take a while so we need to check again if we should stop
          if (IsToStop() || loadingProcessInterface.IsToStop()) break;

          HandleHighPriorityRequestQueue(sFolderPath, files);

          if (IsToStop() || loadingProcessInterface.IsToStop()) break;

          if (sThumbnailFilePath.empty()) LOG<<"cImageLoadThread::ThreadFunction Error creating thumbnail \""<<sFolderPath<<"\""<<std::endl;
          else LoadThumbnailImage(sThumbnailFilePath, sFileNameNoExtension, IMAGE_SIZE::THUMBNAIL);

          iter++;
        }

        //LOG<<"cImageLoadThread::ThreadFunction Loop deleting event"<<std::endl;
        spitfire::SAFE_DELETE(pRequest);

        // Enforce the maximum cache size
        size_t nTempMaximumCacheSizeGB = 0;

        {
          spitfire::util::cLockObject lock(mutexMaximumCacheSize);
          nTempMaximumCacheSizeGB = nMaximumCacheSizeGB;
        }

        cImageCacheManager::EnforceMaximumCacheSize(nTempMaximumCacheSizeGB);
      } else {
        // If the queue is empty then we know that there are no more actions and it is safe to reset our stop loading signal object
        loadingProcessInterface.Reset();
      }

      // Try to avoid hogging the CPU
      spitfire::util::SleepThisThreadMS(1);
      spitfire::util::YieldThisThread();
    }

    {
      // Delete the photos
      std::map<string_t, cPhoto*>::iterator iter = files.begin();
      const std::map<string_t, cPhoto*>::iterator iterEnd = files.end();
      while (iter != iterEnd) {
        spitfire::SAFE_DELETE(iter->second);

        iter++;
      }

      files.clear();
    }

    // Remove any further events because we don't care any more
    ClearEventQueue();

    LOG<<"cImageLoadThread::ThreadFunction returning"<<std::endl;
  }
}
