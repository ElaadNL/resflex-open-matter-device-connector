from esp_docs.conf_docs import *  # noqa: F403,F401

languages = ['en']
idf_targets = ['esp32', 'esp32s3', 'esp32c2', 'esp32c3', 'esp32c6', 'esp32h2', 'esp32p4']

extensions += ['sphinx_copybutton',
               # Needed as a trigger for running doxygen
               'esp_docs.esp_extensions.dummy_build_system',
               'esp_docs.esp_extensions.run_doxygen',
               ]

# link roles config
github_repo = 'ElaadNL/resflex-open-matter-device-connector'

# context used by sphinx_idf_theme
html_context['github_user'] = 'ElaadNL'
html_context['github_repo'] = 'resflex-open-matter-device-connector'

html_static_path = ['../_static']

# Extra options required by sphinx_idf_theme
project_slug = 'esp-matter'

# Contains info used for constructing target and version selector
# Can also be hosted externally, see esp-idf for example
# NOTE: still points at upstream Espressif; the version selector therefore lists
# upstream esp-matter releases, not this fork's. Replace if ElaadNL hosts its own.
versions_url = 'https://docs.espressif.com/projects/esp-matter/en/latest/esp32/_static/esp_sdk_matter_version.js'

# Final PDF filename will contains target and version
pdf_file_prefix = u'esp-docs'
