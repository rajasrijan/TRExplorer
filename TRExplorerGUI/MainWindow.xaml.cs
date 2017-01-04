using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Xml;

namespace TRExplorerGUI
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public class ImageListing
    {
        public string Name { get; set; }
        public string Path { get; set; }
        public ImageListing(string _name, string _path)
        {
            Name = _name;
            Path = System.IO.Path.GetFullPath(_path);
        }
    }
    public class TreeListing
    {
        public string Name { get; set; }
        public int Id { get; set; }
        public string DisplayText
        {
            get
            {
                return Id.ToString() + ". " + Name;
            }
        }
        public TreeListing(string _name, int _id)
        {
            Name = _name;
            Id = _id;
        }
    }

    public partial class MainWindow : Window
    {
        public string tigerFileLocation = null;
        public MainWindow()
        {
            InitializeComponent();
        }

        private void Window_Initialized(object sender, EventArgs e)
        {
            List<ImageListing> items = new List<ImageListing>();
            for (int i = 0; i < 10; i++)
            {
                items.Add(new ImageListing("a", "a.jpg"));
            }
            imageListView.ItemsSource = items;
        }

        private void loadButton_Click(object sender, RoutedEventArgs e)
        {
            tigerFileLocation = tigerFileLocationTextBox.Text;
            if (!File.Exists(tigerFileLocation))
            {
                MessageBox.Show("File [" + tigerFileLocation + "] not found.", "File not found", MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            Process TRExplorer = new Process();
            TRExplorer.StartInfo.FileName = "TRExplorer.exe";
            TRExplorer.StartInfo.Arguments = "\"" + tigerFileLocation + "\" info";
            TRExplorer.StartInfo.UseShellExecute = false;
            TRExplorer.StartInfo.RedirectStandardOutput = true;
            TRExplorer.Start();

            StreamReader reader = TRExplorer.StandardOutput;
            List<TreeListing> lines = new List<TreeListing>();
            int id = 0;
            while (!reader.EndOfStream)
            {
                string line = reader.ReadLine();
                TreeListing drm = new TreeListing(line, id);
                lines.Add(drm);
                id++;
            }

            drmBrowserTree.ItemsSource = lines;
            TRExplorer.WaitForExit();
        }

        private void drmBrowserTree_SelectedItemChanged(object sender, RoutedPropertyChangedEventArgs<object> e)
        {
            TreeListing tl = (TreeListing)drmBrowserTree.SelectedItem;
            Process TRExplorer = new Process();
            TRExplorer.StartInfo.FileName = "TRExplorer.exe";
            TRExplorer.StartInfo.Arguments = "\"" + tigerFileLocation + "\" unpack " + tl.Id.ToString() + " output";
            TRExplorer.StartInfo.UseShellExecute = false;
            TRExplorer.StartInfo.RedirectStandardOutput = true;
            TRExplorer.Start();

            StreamReader reader = TRExplorer.StandardOutput;
            List<ImageListing> items = new List<ImageListing>();
            while (!reader.EndOfStream)
            {
                string line = reader.ReadLine();
                ImageListing image = new ImageListing("test", line);
                items.Add(image);
            }

            TRExplorer.WaitForExit();
            imageListView.ItemsSource = items;
        }

        private void Window_Closed(object sender, EventArgs e)
        {
            Directory.Delete("output", true);
        }
    }
}
