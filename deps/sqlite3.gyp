{
  'includes': ['common.gypi'],
  'target_defaults': {
    'configurations': {
      'Debug': {
        'cflags': [
          '-Wno-unused-function',
        ],
        'xcode_settings': {
          'WARNING_CFLAGS': [
            '-Wno-unused-function',
          ],
        },
        'msvs_settings': {
          'VCCLCompilerTool': {
            'RuntimeLibrary': 1, # static debug
          },
        },
      },
      'Release': {
        'msvs_settings': {
          'VCCLCompilerTool': {
            'RuntimeLibrary': 0, # static release
          },
        },
      },
    },
  },
  'targets': [
    {
      'target_name': 'action_before_build',
      'type': 'none',
      'hard_dependency': 1,
      'actions': [{
        'action_name': 'unpack_sqlite_dep',
        'inputs': ['./sqlite-autoconf-<@(sqlite_version).tar.gz'],
        'outputs': ['<(SHARED_INTERMEDIATE_DIR)/sqlite-autoconf-<@(sqlite_version)/sqlite3.c'],
        'action': ['node', './extract.js', './sqlite-autoconf-<@(sqlite_version).tar.gz', '<(SHARED_INTERMEDIATE_DIR)'],
      }],
    },
    {
      'target_name': 'sqlite3',
      'type': 'static_library',
      'dependencies': ['action_before_build'],
      'sources': ['<(SHARED_INTERMEDIATE_DIR)/sqlite-autoconf-<@(sqlite_version)/sqlite3.c'],
      'include_dirs': ['<(SHARED_INTERMEDIATE_DIR)/sqlite-autoconf-<@(sqlite_version)/'],
      'direct_dependent_settings': {
        'include_dirs': ['<(SHARED_INTERMEDIATE_DIR)/sqlite-autoconf-<@(sqlite_version)/'],
      },
      'cflags': [
        '-std=c99',
        '-Wno-sign-compare',
      ],
      'xcode_settings': {
        'OTHER_CFLAGS': [
          '-std=c99',
        ],
        'WARNING_CFLAGS': [
          '-Wno-sign-compare',
        ],
      },
      'defines': [
        'SQLITE_THREADSAFE=0',
        'SQLITE_DEFAULT_MEMSTATUS=0',
        # TODO: determine if this would be a severe enough breaking change to avoid
        # 'SQLITE_LIKE_DOESNT_MATCH_BLOBS',
        'SQLITE_OMIT_DEPRECATED',
        'SQLITE_OMIT_TRACE',
        'SQLITE_OMIT_COMPLETE',
        'SQLITE_OMIT_GET_TABLE',
        'SQLITE_OMIT_DESERIALIZE',
        'SQLITE_OMIT_TCL_VARIABLE',
        'SQLITE_OMIT_AUTHORIZATION',
        'SQLITE_OMIT_PROGRESS_CALLBACK',
        'SQLITE_DEFAULT_CACHE_SIZE=-16000',
        'SQLITE_DEFAULT_FOREIGN_KEYS=1',
        'SQLITE_DEFAULT_WAL_SYNCHRONOUS=1',
        'SQLITE_MAX_ATTACHED=125',
        'SQLITE_MAX_LENGTH=2147483647',
        'SQLITE_MAX_SQL_LENGTH=1073741824',
        'SQLITE_USE_URI=1',
        'SQLITE_ENABLE_COLUMN_METADATA',
        # TODO: test ANALYZE with stat4 enabled
        # 'SQLITE_ENABLE_STAT4',
        # TODO: evaluate the extra compilation steps necessary for these features
        # 'SQLITE_ENABLE_UPDATE_DELETE_LIMIT',
        'SQLITE_ENABLE_FTS4',
        'SQLITE_ENABLE_FTS5',
        'SQLITE_ENABLE_JSON1',
        'SQLITE_ENABLE_RTREE',
        # TODO: determine if the emitted warnings are safe or not
        # 'SQLITE_ENABLE_GEOPOLY',
        'SQLITE_INTROSPECTION_PRAGMAS',
        'SQLITE_SOUNDEX',
      ],
    },
  ],
}
