from distutils.core import setup, Extension
setup(
    name="hal",
    version="1.0.0",
    author="Benjamin Flin",
    author_email="benjamin.flin@peratonlabs.com",
    description="Hardware Abstraction Layer (HAL)",
    long_description="file: README.md",
    long_description_content_type="text/markdown",
    url = "https://github.com/gaps-closure/hal",
    project_urls={
        "Bug Tracker": "https://github.com/gaps-closure/hal/issues"
    },
    classifiers=["Programming Language :: Python :: 3"],
    python_requires=">=3.6",
    packages=["autogen"],
    install_requires = [
        "libconf",
        "lark"
    ],
    entry_points={
        "console_scripts": [
            "hal_autogen=autogen:autogen.main",
        ]
    },
    scripts=[
        'confgen/hal_autoconfig.py',
        'confgen/merge_xdconf_ini.py',
    ]
)