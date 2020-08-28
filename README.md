dinamo
=====

An C++ boilerplate for developing with Cocos2d-x. To start developing:

``
git submodule update --init
``

``
cd cocos2d/
``

``
git submodule update --init tools/cocos2d-console
``

``
./download-deps.py
``

``
./setup.py
``

And run CMake for the platform you're running, like:

``
mkdir mac-build
``

``
cd mac-build
``

``
cmake -G Xcode ..
``

Or use cocos console tool:

``
cocos compile . -p mac -m release
``