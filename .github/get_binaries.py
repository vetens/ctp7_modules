import os
from urllib2 import urlopen, URLError, HTTPError
from ast import literal_eval
from optparse import OptionParser

parser = OptionParser()
parser.add_option("-t", "--tag", dest="tag",
                  help="Tag name", metavar="tag")
parser.add_option("-l", "--uploads", dest="uploads",
                  help="List of files to upload\n Should be a dict:\n {\"filename\":\"path_to_file\"}", metavar="uploads")

def dlfile(url, local_path):
    # Open the url
    try:
        f = urlopen(url)
        print "downloading " + url

        # Open our local file for writing
        with open(local_path, "wb") as local_file:
            local_file.write(f.read())

    #handle errors
    except HTTPError, e:
        print "HTTP Error:", e.code, url
    except URLError, e:
        print "URL Error:", e.reason, url


def main():
    # parse command line options
    (options,args) = parser.parse_args()
    base_url = "https://github.com/cms-gem-daq-project/ctp7_modules/releases/download/"
    uploads_dict = {}
    with open(options.uploads,'r') as f:
      uploads_dict = literal_eval(f.read())
    for key,val in uploads_dict.iteritems():
        url = base_url+options.tag+"/"+key
        local_path = val.replace('$CTP7_MOD_ROOT', os.getenv('CTP7_MOD_ROOT'))
        dlfile(url, local_path)

if __name__ == '__main__':
    main()
