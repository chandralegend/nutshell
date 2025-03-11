class Nutshell < Formula
  desc "Enhanced Unix shell with simplified command language and AI assistance"
  homepage "https://github.com/chandralegend/nutshell"
  url "https://github.com/chandralegend/nutshell/archive/refs/tags/v0.0.4.tar.gz"
  sha256 "d3cd4b9b64fb6d657195beb7ea9d47a193ace561d8d54b64e9890304e41c6829"
  license "MIT"
  head "https://github.com/chandralegend/nutshell.git", branch: "main"

  depends_on "pkg-config" => :build
  depends_on "jansson"
  depends_on "readline"
  depends_on "openssl@3"
  depends_on "curl"
  
  def install
    # Pass correct environment variables to find libraries
    ENV.append "CFLAGS", "-I#{Formula["jansson"].opt_include}"
    ENV.append "LDFLAGS", "-L#{Formula["jansson"].opt_lib} -ljansson"
    ENV.append "CFLAGS", "-I#{Formula["openssl@3"].opt_include}"
    ENV.append "LDFLAGS", "-L#{Formula["openssl@3"].opt_lib}"
    
    system "make"
    bin.install "nutshell"
    
    # Install documentation
    doc.install "README.md", "CHANGELOG.md"
    
    # Create themes directory and install themes directly in the Cellar
    # The themes directory will be in the Formula's prefix, not in /usr/local/share
    themes_dir = prefix/"themes"
    themes_dir.mkpath
    Dir["themes/*.json"].each do |theme_file|
      themes_dir.install theme_file
    end
    
    # Create a nutshell config directory in the Formula's prefix for packages
    (prefix/"packages").mkpath
  end

  def post_install
    # Create ~/.nutshell directory structure for the user if it doesn't exist
    user_config_dir = "#{Dir.home}/.nutshell"
    user_themes_dir = "#{user_config_dir}/themes"
    user_packages_dir = "#{user_config_dir}/packages"
    
    system "mkdir", "-p", user_themes_dir
    system "mkdir", "-p", user_packages_dir
    
    # Copy themes to user directory if it doesn't already have them
    if Dir.exist?(user_themes_dir) && Dir.empty?(user_themes_dir)
      Dir["#{prefix}/themes/*.json"].each do |theme|
        system "cp", theme, user_themes_dir
      end
    end
    
    # Print instructions for the user
    ohai "Nutshell has been installed!"
    opoo "Make sure to set an API key for AI features with: set-api-key YOUR_API_KEY"
  end

  test do
    # Test that nutshell runs without errors (--help should return 0)
    assert_match "Nutshell", shell_output("#{bin}/nutshell --help", 0)
  end
end
