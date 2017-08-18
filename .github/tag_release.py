# Script to tag the code, prepare a release and push it to the github
from optparse import OptionParser
from subprocess import call
from sys import exit
from ast import literal_eval

parser = OptionParser()
parser.add_option("-u", "--user", dest="user",
                  help="GitHub user name", metavar="user")
parser.add_option("-r", "--repo", dest="repo",
                  help="GitHub repo name", metavar="repo")
parser.add_option("-t", "--tag", dest="tag",
                  help="Tag name", metavar="tag")
parser.add_option("-n", "--release_name", dest="release_name",
                  help="Release name, defaults to tag name", metavar="release_name")
parser.add_option("-d", "--description", dest="description",
                  help="Release description file name", metavar="description")
parser.add_option("-p", "--prerelease", action="store_true", dest="prerelease",
                  help="Mark as pre-release", metavar="prerelease")
parser.add_option("-l", "--uploads", dest="uploads",
                  help="List of files to upload\n Should be a dict:\n {\"filename\":\"path_to_file\"}", metavar="uploads")
parser.add_option("-v", "--verbose", action="store_true", dest="verbose",
                  help="Verbosity level", metavar="verbose")

def create_tag(tag, verbose):
  if verbose:
    print "Existing tags"
    command = "git tag -l"
    call(command, shell=True)
    print "Adding tag %s" %(tag)
  command = "git tag %s && git push --tags" %(tag)
  call(command, shell=True)

def create_release(user, repo, tag, name, description, pre, verbose):
  if verbose:
    print "Existing releases"
    command = "github-release info -u %s -r %s" %(user,repo)
    call(command, shell=True)
    print "Adding release %s" %(name)
  if pre:
    command = "github-release release -u %s -r %s --tag %s --name \"%s\" --description \"%s\" -p" %(user,repo,tag,name,description)
  else:
    command = "github-release release -u %s -r %s --tag %s --name \"%s\" --description \"%s\"" %(user,repo,tag,name,description)
  call(command, shell=True)
  command = "github-release info -u %s -r %s" %(user,repo)
  call(command, shell=True)

def upload_file(user, repo, tag, name, path, verbose):
  if verbose:
    print "Upload file %s" %(path)
  command = "github-release upload -u %s -r %s -t %s -n %s -f %s" %(user,repo,tag,name,path)
  call(command, shell=True)
  
def main():
  # parse command line options
  (options,args) = parser.parse_args()
  # process options
  # require that at least username, repo and tag are provided
  if options.tag is None or options.user is None or options.repo is None:
    print "Please provide a user name, repository name and tag"
    parser.print_help()
    exit(0)
  
  if options.release_name is None:
    rn = options.tag
  else:
    rn = options.release_name

  if options.description is None:
    desc = "None"
  else:
    with open(options.description,'r') as f:
      desc = f.read()

  create_tag(options.tag, options.verbose)
  create_release(options.user, options.repo, options.tag, rn, desc, options.prerelease, options.verbose)

  if options.uploads is not None:
    uploads_dict = {}
    with open(options.uploads,'r') as f:
      uploads_dict = literal_eval(f.read())
    for key,val in uploads_dict.iteritems():
      upload_file(options.user, options.repo, options.tag, key, val, options.verbose)

if __name__ == "__main__":
  main()
