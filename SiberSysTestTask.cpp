
#include "stdafx.h"
#include <queue>
#include <string>
#include <vector>
#include <set>
#include <Windows.h>

using namespace std;


const char * csHtmlHeader = "<!DOCTYPE html><html><head>\n"  \
	"<style type=\"text/css\">\n" \
	"td { border: 0px;}\n" \
	"table { border-collapse: collapse;}\n" \
	"</style>\n" \
	"</head>\n" \
	"<body>\n" \
	"<table style=\"border-style: 0; border-width: 0px; width:100%%; height: 100%%; position: absolute;\">\n";

const char * csHtmlTail =     "</table>\n" \
	"</body>\n" \
	"</html>\n";

const char * csHtmlTr = "<tr>";
const char * csHtmlTr_ = "</tr>\n";
const char * csHtmlTd	   = "<td width=\"%d%%\" height=\"%d%%\" style=\"background-color:%s\">&nbsp;</td>";
const char * csHtmlTd_next = "<td width=\"%d%%\" style=\"background-color:%s\">&nbsp;</td>";



//colors for rect without backgrand
const char *colors_rect[] = {"#000000","#FF0000","#00FF00","#0000FF","#FFFF00","#00FFFF","#FF00FF","#C0C0C0"/*,"#FFFFFF"*/};

//
//auxiliary function to print trace into error stream 
//
void PrtLog(const char* fmt_str, ...)
{
	char buf[1024];
	va_list marker;
	va_start (marker, fmt_str );
	vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt_str, marker );
	va_end  ( marker );
	fprintf( stderr, buf );
}

//main class that draws rects in html
class ShowRectsViaTable
{
public:
	vector<RECT> rects;
	set<int> set_x;
	set<int> set_y;

public:
	//read corresponded stream to read rectangles 
	int readRects(FILE *filestr)
	{
		//reads input stream to match by corresponded pattern
		int len = 0x200; // maximum symbols in line
		char *str = (char*)malloc(len);
		if(!str) 
		{ PrtLog("Error: Out of memory"); return -1; }

		//reads all input data and keeps it
		for(int line=1;0 != fgets(str,len,filestr);line++)
		{
			char *p = str;

			RECT r;int n=0;
			try{
				n = sscanf_s(str,"%d,%d,%d,%d",&r.left,&r.top,&r.right,&r.bottom);
			}catch(...){
				PrtLog("Exception in line (%d): %s\n",line,str);
			}

			if(n!=4)
			{
				PrtLog("Incorrect read line (%d): %s\n",line,str);
				continue;
			}

			rects.push_back(r);
			set_x.insert(r.left);
			set_x.insert(r.right);
			set_y.insert(r.top);
			set_y.insert(r.bottom);
		}
		init_convert();
		return rects.size();
	}

	//generates html file
	int genHtml(FILE *file)
	{
		//prints header of html
		fprintf(file,csHtmlHeader);

		//enum all available x and y
		//and try to cross each rectangle(x[i],y[j],x[i+1],y[j+1]) with our rects
		//if it is crossed fill as rect, otherwise fill background
		set<int>::iterator it_y; int iy;
		for(it_y=set_y.begin(),
			iy=0;
			iy < (int)set_y.size()-1;
		iy++ )
		{
			RECT r;
			r.top = *it_y;
			if((++it_y) == set_y.end())
				break;
			r.bottom = *it_y;

			//
			int delta_y= convert(r.bottom-r.top);
			fprintf(file, csHtmlTr);

			set<int>::iterator it_x; int ix;
			for(it_x=set_x.begin(),ix=0;ix<(int)set_x.size()-1;ix++)
			{
				r.left = *it_x;
				if((++it_x) == set_x.end())
					break;
				r.right = *it_x;

				//check that r is not crossed with any our rects
				int indxRect;
				//if crossed we fill it by one from our available colors in colors_rect, otherwise 'white'
				const char *sColor = isCrossed(r,indxRect) ? colors_rect[indxRect%(sizeof(colors_rect)/sizeof(*colors_rect))] : "#FFFFFF";
				int delta_x=convert(r.right-r.left);
				if(ix==0)
					fprintf(file, csHtmlTd, delta_x, delta_y, sColor);
				else
					fprintf(file, csHtmlTd_next, delta_x, sColor);

			}

			fprintf(file,csHtmlTr_);
		}
		//prints tail of html
		fprintf(file,csHtmlTail);

		return 0;
	}
private:

	//checks crossing r with all rects
	bool isCrossed(const RECT &r, int &indxRect)
	{
		indxRect=0;
		//it works simply because we know that r has been constructed from coordinates of rects
		for(vector<RECT>::iterator it=rects.begin();it!=rects.end();it++,indxRect++)
		{
			if ( r.left >= it->left && r.right <= it->right && r.top >= it->top && r.bottom <= it->bottom )
			{
				return true;
			}
		}
		return false;
	}

	int _delta;

	//init convert. it is called after read rects and before gen html
	void init_convert()
	{
		int _min_x=*set_x.begin();
		int _max_x=*(--set_x.end());
		int _min_y=*set_y.begin();
		int _max_y=*(--set_y.end());
		int delta_x = _max_x-_min_x;
		int delta_y = _max_y-_min_y;
		_delta = delta_x>delta_y? delta_x : delta_y;
	}

	//convert x or y to %
	int convert(int n)
	{
		return MulDiv(n,100,_delta);
	}

};



int main(int argc, char* argv[])
{
	ShowRectsViaTable tbl;
	if( argc != 1 )
	{
		FILE *f;
		f = fopen(argv[1],"r");
		if(f)
		{
			tbl.readRects(f);
			fclose(f);
		}
	}

	if( tbl.rects.size() == 0 )
		return 0;

	FILE *f;
	fopen_s(&f,"out.html","wt");
	if(f)
	{
		tbl.genHtml(f);
		fclose(f);
	}

	return 0;
}

