project('adapter-guard', 'c',
  version: '0.1.0',
)

libusb = dependency('libusb-1.0', required: true)

executable('adapter-guard',
  sources: [
    'src/cmd.c',
    'src/adapter-guard.c',
    'src/main.c',
  ],
  dependencies: [
    libusb,
  ],
  install: true,
  install_dir: '/usr/bin',
)
