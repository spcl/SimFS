"""
Python wrapper module to access the extended API functions that are provided by the
re-loaded DVLib wrapper and loaded into the python program by either
NetCDF or HDF5 module.
v0.1 2017-07-22 Pirmin Schmid

/**
  * access a pre-defined key-value store in DV
  */
void sdavi_set_info(const char *key, int64_t value);
int64_t sdavi_get_info(const char *key);

// return: 0 ok, error otherwise
int64_t sdavi_request_range(const char *first, char *last);


/**
 * returns value
 *  0 -> not available / not scheduled
 *  1 -> simulation for file is scheduled
 *  2 -> file is being produced in running simulation
 *  3 -> file exists
 */
int64_t sdavi_test_file(const char *path);


/**
 * return value:
 *  0 -> OK
 * -1 -> unspecified error that prevents normal action (fallback)
 * -2 -> cache full preventing additional jobs to be launched at the moment (wait can help)
 */
int64_t sdavi_status();
"""

import ctypes

UNKNOWN = 0
NETCDF  = 1
PNETCDF = 2
HDF5_8  = 3
HDF5_10 = 4
PHDF5   = 5

def assure_path_with_slash(path):
	path = str(path).rstrip()                                                                     
	while path[-1] == '/':                                                                        
		path = path[:len(path)-1]                                                             
	return path + '/' 


class DVLibExtendedAPI:
	dvlib_type = UNKNOWN
	dvlib = None
	_set_info = None
	_get_info = None
	_request_range = None
	_test_file = None
	_status = None


	def __init__(self, dvlib_type, path):
		# assure proper path format
		if len(path) == 0:
			path = './'
		path = assure_path_with_slash(path)

		self.dvlib_type = dvlib_type
		if dvlib_type == NETCDF:
			self.dvlib = ctypes.cdll.LoadLibrary(path + 'libdvl.so')
		elif dvlib_type == PNETCDF:
			self.dvlib = ctypes.cdll.LoadLibrary(path + 'libdvlp.so')
		elif dvlib_type == HDF5_8:
			self.dvlib = ctypes.cdll.LoadLibrary(path + 'lidvlh8.so')
		elif dvlib_type == HDF5_10:
			self.dvlib = ctypes.cdll.LoadLibrary(path + 'libdvlh10.so')
		else:
			print 'DVLibExtendedAPI: unknown dvlib_type ' + dvlib_type
			exit()

		# int _set_info(str, int)
		self._set_info = self.dvlib.sdavi_set_info
		self._set_info.argtypes = [ctypes.c_char_p, ctypes.c_int]
		self._get_info.restype = ctypes.c_int

		# int _get_info(str, int)
		self._get_info = self.dvlib.sdavi_get_info
		self._get_info.argtypes = [ctypes.c_char_p, ctypes.c_int]
		self._get_info.restype = ctypes.c_int

		# int _request_range(str, str)
		self._request_range = self.dvlib.sdavi_request_range
		self._request_range.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int]
		self._request_range.restype = ctypes.c_int

		# int _request_multiple_ranges(str_arr, str_arr, int_arr, int)
		self._request_multiple_ranges = self.dvlib.sdavi_request_multiple_ranges
		# argtypes generated during call
		self._request_range.restype = ctypes.c_int

		# int _test_file(str)
		self._test_file = self.dvlib.sdavi_test_file
		self._test_file.argtypes = [ctypes.c_char_p]
		self._test_file.restype = ctypes.c_int		

		# int _status(void)
		self._status = self.dvlib.sdavi_status
		self._status.argtypes = []
		self._status.restype = ctypes.c_int


	def sdavi_set_info(self, key, value):
		self._set_info(key, value)

	def sdavi_get_info(self, key):
		return self._get_info(key)

	def sdavi_request_range(self, first, last, stride):
		return self._request_range(first, last, stride)

	def sdavi_request_multiple_ranges(self, first_list, last_list, strides_list, length):
		if length < 0:
			return -1
		if length == 0:
			return 0;
		if len(first_list) != length:
			print 'error: len(first_list) != length'
			return -1
		if len(last_list) != length:
			print 'error: len(last_list) != length'
			return -1
		if len(strides_list) != length:
			print 'error: len(strides_list) != length'
			return -1
		self._request_multiple_ranges.argtypes = [ctypes.c_char_p * length, ctypes.c_char_p * length, c_int * length, c_int]
		return self._request_multiple_ranges(first_list, last_list, strides_list, length)

	def sdavi_test_file(self, path):
		return self._test_file(path)

	def sdavi_status(self):
		return self._status()
