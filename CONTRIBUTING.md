# Contributing to PaasPhoos

Thank you for your interest in contributing to **PaasPhoos**!

## Real-Time Audio Constraints
When contributing code to PaasPhoos, remember the golden rules of real-time audio development:
1. **Never allocate heap memory on the audio thread**: No `new`, `malloc`, or resizing standard containers (`std::vector::push_back`, `std::string`, etc.) inside `audioDeviceIOCallbackWithContext`.
2. **Never lock mutexes or sleep on the audio thread**: Use lock-free circular ring buffers (`AudioRingBuffer`, `juce::AbstractFifo`) and atomic primitives (`std::atomic<float>`, `std::atomic<bool>`).
3. **Keep code deterministic and zero-cost**: Audio callbacks execute every 1 to 5 milliseconds. Any priority inversion or page fault causes audible glitches.

## Building and Testing Locally
1. Run `.\build.bat` (or `.\build.ps1`).
2. Run the test suite:
   ```cmd
   cmake --build build --config Release --target PaasPhoosTests
   .\build\Release\PaasPhoosTests.exe
   ```
3. Ensure all tests pass before submitting a Pull Request.

## Pull Request Guidelines
- Follow C++17 modern idioms.
- Keep UI components decoupled from real-time audio processors.
- Explain testing methodology and latency measurements in PR descriptions.
