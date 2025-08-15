project('Euteran', 'c',
  version: '1.0.0',
  default_options: [
    'c_std=c23', 
    'warning_level=1',
  ]
)

add_project_arguments([
  '-D_GNU_SOURCE',
  '-D_POSIX_C_SOURCE=200809L',
  '-D_DEFAULT_SOURCE',
], language: 'c')

cc = meson.get_compiler('c')

if cc.has_header('alloca.h')
  add_project_arguments('-DHAVE_ALLOCA_H', language: 'c')
endif

if cc.has_header('locale.h')
  add_project_arguments('-DHAVE_LOCALE_H', language: 'c')
endif
gtk_dep = dependency('gtk4')
adw_dep = dependency('libadwaita-1')
mpg123_dep = dependency('libmpg123')

deps = [gtk_dep, adw_dep, mpg123_dep]

if host_machine.system() == 'linux'
  pipewire_dep = dependency('libpipewire-0.3')
  xcb_dep = dependency('xcb')
  deps += [pipewire_dep, xcb_dep]
elif host_machine.system() == 'windows'
  portaudio_dep = dependency('portaudio-2.0')
  deps += [portaudio_dep]
endif

blueprint_compiler = find_program('blueprint-compiler')

blueprints = files(
  'ui/euteran_main.blp',
  'ui/euteran_audio_devices.blp'
)

ui_files = []
foreach blueprint : blueprints
  ui_files += custom_target(
    blueprint.full_path().split('/')[-1],
    input: blueprint,
    output: '@BASENAME@.ui',
    command: [blueprint_compiler, 'compile', '@INPUT@', '--output', '@OUTPUT@'],
  )
endforeach

gnome = import('gnome')
resources = gnome.compile_resources(
  'euteran-resources',
  configure_file(
    input: 'src/euteran.gresource.xml.in',
    output: 'euteran.gresource.xml',
    copy: true
  ),
  dependencies: ui_files,
  source_dir: meson.current_build_dir(),
  c_name: 'euteran'
)

sources = []

if host_machine.system() == 'windows'
  sources = files(
    'main.c',
    'src/eut_utils.c',
    'src/ogg/stb_vorbis.c'
  )
else
  sources = files(
    'src/main.c',
    'src/eut_async.c',
    'src/eut_audiodevices.c',
    'src/eut_audioinput.c',
    'src/eut_audiolinux.c',
    'src/eut_binaryparser.c',
    'src/eut_dialogs.c',
    'src/eut_foldercb.c',
    'src/eut_logs.c',
    'src/eut_main_object.c',
    'src/eut_musiclistfunc.c',
    'src/eut_settings.c',
    'src/eut_subwindow.c',
    'src/eut_subwindowimpl.c',
    'src/eut_utils.c',
    'src/eut_widgetsfunctions.c',
    'src/ogg/stb_vorbis.c',
    'src/wav/dr_wav.c'
  )
endif

executable('Euteran',
  sources + resources,
  dependencies: deps,
  include_directories: include_directories('src'),
  link_args: ['-lm'], 
  install: true
)

style_copy = custom_target('copy-style',
  input: 'meson.build',  
  output: 'style-copied', 
  command: [
    meson.current_source_dir() / 'copy_style.py',
    meson.current_source_dir() / 'Style',
    meson.current_build_dir() / 'Style'
  ],
  build_by_default: true,
  build_always_stale: true
)
