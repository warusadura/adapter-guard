project('adapter-guard', 'c',
  version: '0.1.0',
)

libusb = dependency('libusb', required: true)
executable('adapter-guard',
  sources: [
    'src/main.c',
  ],
  dependencies: [
    libusb,
  ],
  install: true,
  install_dir: '/usr/bin',
)

