import distutils.core as dst

shm_module = dst.Extension(
    '_py_shm', 
    sources = ['_py_shm.c']
)

dst.setup(
    name = '_py_shm', 
    version = '1.0', 
    description = 'store a list in a shared memory segment',
    ext_modules = [shm_module]
)
