name='powersaving-quick-setting'

powersaving_quick_setting_resources = gnome.compile_resources(
  'phosh-plugin-powersaving-quick-setting-resources',
  'phosh-plugin-powersaving-quick-setting.gresources.xml',
  c_name: 'phosh_plugin_powersaving_quick_setting',
)

powersaving_quick_setting_plugin_sources = files(
  'phosh-plugin-powersaving-quick-setting.c',
  'powersaving-quick-setting.c',
)

phosh_powersaving_quick_setting_plugin = shared_module(
  'phosh-plugin-powersaving-quick-setting',
  powersaving_quick_setting_plugin_sources,
  powersaving_quick_setting_resources,
  c_args : ['-DG_LOG_DOMAIN="phosh-plugin-@0@"'.format(name),
            '-DPLUGIN_NAME="@0@"'.format(name),
           ],
  dependencies: plugin_dep,
  install: true,
  install_dir: plugins_dir,
)

pluginconf = configuration_data()
pluginconf.set('name', name)
pluginconf.set('plugins_dir', plugins_dir)

i18n.merge_file(
  input: configure_file(
    input: name + '.desktop.in.in',
    output: name + '.desktop.in',
    configuration: pluginconf,
  ),
  output: name + '.plugin',
  po_dir: join_paths(meson.project_source_root(), 'po'),
  install: true,
  install_dir: plugins_dir,
  type: 'desktop'
)
