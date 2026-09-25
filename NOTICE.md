# NOTICE

**Trivial Engine**
Copyright 2026 Tobias Sharman
<https://github.com/Tobias-Sharman/Trivial-Game-Engine>

This product is licensed under the [Apache License, Version 2.0](LICENSE).
This NOTICE file must be reproduced, per section 4(d) of that license, in any
redistribution of this software or works derived from it — in source form,
binary/object form, or (where such notices are otherwise displayed) within
documentation or an in-application credits screen.

## Third-Party Dependencies

The following are pulled in via CMake `FetchContent` and statically linked
into Trivial Engine (see [CMakeLists.txt](CMakeLists.txt)). If you distribute
Trivial Engine, or a game built with it, you must also carry forward the
notices below for whichever of these are statically linked into what you
distribute.

### GLFW

- **Repository:** <https://github.com/glfw/glfw>
- **Version:** 3.5.1
- **License:** zlib/libpng license

```text
Copyright (c) 2002-2006 Marcus Geelnard

Copyright (c) 2006-2019 Camilla Löwy

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would
   be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and must not
   be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
   distribution.

```

### Vulkan Memory Allocator (VMA)

- **Repository:** <https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator>
- **Version:** v3.4.0
- **License:** MIT
- Linked privately into Trivial Engine; not exposed as part of the public API.

```text
Copyright (c) 2017-2026 Advanced Micro Devices, Inc. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
```

### Tracy Profiler

- **Repository:** <https://github.com/wolfpld/tracy>
- **Version:** v0.13.0
- **License:** BSD 3-Clause

```text
Tracy Profiler (https://github.com/wolfpld/tracy) is licensed under the
3-clause BSD license.

Copyright (c) 2017-2025, Bartosz Taudul <wolf@nereid.pl>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```

### Vulkan SDK

Trivial Engine links against the Vulkan loader via `find_package(Vulkan
REQUIRED)`. This is not vendored or fetched by this repository — it must be
installed separately (e.g. via the LunarG Vulkan SDK) and is used under its
own license terms, not redistributed as part of Trivial Engine's source.

### Google Test

- **Repository:** <https://github.com/google/googletest>
- **License:** BSD 3-Clause
- Used only to build and run this repository's own test suite
([tests/CMakeLists.txt](tests/CMakeLists.txt)). Not linked into the engine
library and not distributed with it or with games built on it.

```text
Copyright 2008, Google Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

    * Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the
distribution.
    * Neither the name of Google Inc. nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```

## Design References

Of the below projects they are not copied or linked into Trivial Engine, but
due to the influence had on the development of components of the engine it is
found appropriate to give acknowledgment even if not required by licenses.

### parking_lot (Rust)

- **Repository:** <https://github.com/Amanieu/parking_lot>
- **Author:** Amanieu d'Antras
- **License:** dual MIT / Apache-2.0

Trivial Engine's synchronization primitives (SpinLock, Mutex,
ConditionVariable, and the underlying ParkingLot implementation) were designed
with reference to parking_lot's architecture. The approach taken by the engine
is more narrow in application to fit more readily the engines needs without
any (while quite minor in many, if not most, cases) extra overhead for support
beyond what is necessary. Further on the implementation, the form of the
underlying locking is highly subject to change upon a migration to the planned
fibre backed task system. The C++ implementation in this repository is thus
taken as original, not a translation or port of parking_lot's Rust source.
