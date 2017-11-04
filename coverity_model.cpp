// This is the modeling file for Coverity to avoid false positives.
// This code is not supposed to be compiled into callgen323!

struct _ios_fields{
	int _precision;
};
class ios : public _ios_fields{
	
	int precision() const { return _precision; }
	int precision(int newp) {return _precision; }
	
};

