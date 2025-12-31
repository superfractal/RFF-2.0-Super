# RFF 2.0 Super

## Project Status
This repository is a **modified version** of the original RFF project. The main focus of this branch is to improve memory management and stability while maintaining the core performance features.

## RFF 2.0 Improvements

RFF 2.0 has been modified to significantly improve stability and memory efficiency.
> [!NOTE]
> As a trade-off for better memory management, rendering speed in this version is slightly slower than the standard RFF.

### Key Changes
* **Optimized Memory Usage:** * Significantly reduced `MPATable` memory consumption, even when using "No compression" settings.
    * Drastic reduction in `std::bad_alloc` errors during Period calculations using virtual memory.
* **Enhanced UX & Safety:**
    * **Accidental Operation Prevention:** Mouse-based position manipulation is now disabled during keyframe rendering to prevent unintended interruptions.
    * **Revised Layout:** Updated the Explore screen button layout; the Cancel and Reset buttons have been moved to prevent accidental resets during Recompute.
* **Better Defaults:** Initial settings have been adjusted to provide a more user-friendly experience out of the box.

### Performance Tuning
* **LightMandelbrotPerturbator.cpp:** Provides approximately 10% faster rendering compared to the standard RFF.
* **MPATable.h:** Modifying this file can further reduce memory usage when using No compression.
