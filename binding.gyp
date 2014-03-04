{
    'targets': [
    {
            'target_name': 'constantlight',
            'cflags!': [ '-fno-exceptions' ],
            'cflags_cc!': [ '-fno-exceptions' ],
            'conditions': [
                ['OS=="mac"', {
                   'xcode_settings': {
                       'GCC_ENABLE_CPP_EXCEPTIONS': 'YES'
                   }
                }]
            ],
            'dependencies': [
               './src/deps/Hummus/binding.gyp:hummus',
               './src/deps/LibJpeg/binding.gyp:libjpeg'
            ],
            'include_dirs': [
                './src',
                './src/deps/Hummus/',
                './src/deps/LibJpeg/'
            ],
           'sources': [
                './src/ConstantLight.cpp'
            ]

	}

    ]        
}
