name='weather-status-icon'

gweather_dep = dependency('gweather4')
geoclue_dep = dependency('libgeoclue-2.0')

weather_status_icon_resources = gnome.compile_resources(
  'phosh-plugin-weather-status-icon-resources',
  'phosh-plugin-weather-status-icon.gresources.xml',
  c_name: 'phosh_plugin_weather_status_icon',
)

weather_status_icon_plugin_sources = files(
  'phosh-plugin-weather-status-icon.c',
  'weather-status-icon.c',
)

phosh_weather_status_icon_plugin = shared_module(
  'phosh-plugin-weather-status-icon',
  weather_status_icon_plugin_sources,
  weather_status_icon_resources,
  c_args : ['-DG_LOG_DOMAIN="phosh-plugin-@0@"'.format(name),
            '-DPLUGIN_NAME="@0@"'.format(name),
           ],
  dependencies: [plugin_dep, gweather_dep, geoclue_dep],
  install: true,
  install_dir: status_icons_plugins_dir,
)

pluginconf = configuration_data()
pluginconf.set('name', name)
pluginconf.set('plugins_dir', status_icons_plugins_dir)

i18n.merge_file(
  input: configure_file(
    input: name + '.desktop.in.in',
    output: name + '.desktop.in',
    configuration: pluginconf,
  ),
  output: name + '.plugin',
  po_dir: join_paths(meson.project_source_root(), 'po'),
  install: true,
  install_dir: status_icons_plugins_dir,
  type: 'desktop'
)
