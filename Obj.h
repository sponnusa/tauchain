//class Obj {
//public:
static Map<String, Object> newMap() {
	return LinkedHashMap<String, Object>();
}

/**
    Helper function for creating maps and tuning them as necessary.

    @param key
              A key to add to the map on creation.
    @param value
              A value to attach to the key in the new map.
    @return A new {@link Map} instance.
*/
static Map<String, Object> newMap ( String key, Object value ) {
	Map<String, Object> result = newMap();
	result.put ( key, value );
	return result;
}

/**
    Used to make getting values from maps embedded in maps embedded in maps
    easier TODO: roll out the loops for efficiency

    @param map
              The map to get a key from
    @param keys
              The list of keys to attempt to get from the map. The first key
              found with a non-null value is returned, or if none are found,
              the original map is returned.
    @return The key from the map, or the original map if none of the keys are
           found.

    public: static Object get ( Map<String, Object> map, String... keys ) {
	Map<String, Object> result = map;
	for ( const String key : keys ) {
		result = map.obj().get ( key );
		// make sure we don't crash if we get a null somewhere down the line
		if ( result == null )
			return result;
	}
	return result;
    }
*/

static Object put ( Object map, String key1, Object value ) {
	map.obj().put ( key1, value );
	return map;
}

static Object put ( Object map, String key1, String key2, Object value ) {
	map.obj().get ( key1 ).obj( ).put ( key2, value );
	return map;
}

static Object put ( Object map, String key1, String key2, String key3, Object value ) {
	map.obj().get ( key1 ) .obj() .get ( key2 ).obj() .put ( key3, value );
	return map;
}

static Object put ( Object map, String key1, String key2, String key3, String key4,
                    Object value ) {
	map.obj() .get ( key1 ).obj( ).get ( key2 ).obj( ).get ( key3 ).obj( ).put ( key4, value );
	return map;
}
/*
    public: static boolean contains ( Object map, String... keys ) {
		for ( const String key : keys ) {
			map = ( map.obj() ).get ( key );
			if ( map == null )
				return false;
		}
		return true;
	}
*/
static Object remove ( Object map, String k1, String k2 ) {
	return map.obj().get ( k1 ).obj().remove ( k2 );
}

/**
    A null-safe equals check using v1.equals(v2) if they are both not null.

    @param v1
              The source object for the equals check.
    @param v2
              The object to be checked for equality using the first objects
              equals method.
    @return True if the objects were both null. True if both objects were not
           null and v1.equals(v2). False otherwise.
*/
static boolean equals ( Object v1, Object v2 ) {
	return v1 == null ? v2 == null : v1.equals ( v2 );
}
//};
