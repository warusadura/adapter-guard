glib = dependency('glib-2.0', required: true)

test_common_bin = executable('adapter-guard-common',
  sources: [
    'test_common.c',
    '../adapter-guard.c',
    '../adapter-guard-scanner.c',
  ],
  dependencies: [
    glib,
    libsystemd,
    libusb,
  ]
)

test_cmd_bin = executable('adapter-guard-cmd',
  sources: [
    'test_cmd.c',
    '../adapter-guard.c',
    '../adapter-guard-scanner.c',
  ],
  dependencies: [
    glib,
    libsystemd,
    libusb,
  ]
)

test('adapter-guard-common', test_common_bin)
test('adapter-guard-cmd', test_cmd_bin)
