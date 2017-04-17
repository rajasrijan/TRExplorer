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
using System.Drawing;

namespace TRExplorerGUI
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public class ImageListing
    {
        public string Name { get; set; }
        public string Path { get; set; }
        public int Id { get; set; }
        public ImageListing(string _name, string _path, int _id)
        {
            Name = "Texture";
            Id = _id;
            if (_path != null)
            {
                Path = System.IO.Path.GetFullPath(_path);
            }
            else
            {
                Path = "";
            }
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
        public string Path
        {
            get
            {
                return Name;
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
        List<TreeListing> lines;
        public MainWindow()
        {
            InitializeComponent();
        }

        private void Window_Initialized(object sender, EventArgs e)
        {
            if (Properties.Settings.Default.tigerFilePath != null && Properties.Settings.Default.tigerFilePath != "")
            {
                tigerFileLocationTextBox.Text = Properties.Settings.Default.tigerFilePath;

            }
            //List<ImageListing> items = new List<ImageListing>();
            imageListView.Items.Clear();
            for (int i = 0; i < 3; i++)
            {
                imageListView.Items.Add(new ImageListing("a", "a.jpg", -1));
            }
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

            lines = new List<TreeListing>();
            int id = 0;
            while (!reader.EndOfStream)
            {
                string line = reader.ReadLine();
                TreeListing drm = new TreeListing(line, id);
                lines.Add(drm);
                drmBrowserTree.Items.Add(drm);
                id++;
            }
            TRExplorer.WaitForExit();
            Properties.Settings.Default.tigerFilePath = tigerFileLocation;
            Properties.Settings.Default.Save();
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
            imageListView.Items.Clear();
            while (!reader.EndOfStream)
            {
                string line = reader.ReadLine();
                if (line.EndsWith(".dds"))
                {
                    string[] tokens = line.Split(',');
                    int id = Convert.ToInt32(tokens[0].Trim());
                    string path = tokens[1].Trim();
                    ImageListing image = new ImageListing("Texture", path, id);
                    imageListView.Items.Add(image);
                }
            }
            TRExplorer.WaitForExit();
        }

        private void Window_Closed(object sender, EventArgs e)
        {
            //Directory.Delete("output", true);
        }

        private void searchBtn_Click(object sender, RoutedEventArgs e)
        {
            if (searchBox.Text == null || searchBox.Text == "")
            {
                drmBrowserTree.Items.Clear();
                foreach (TreeListing item in lines)
                {
                    drmBrowserTree.Items.Add(item);
                }
                return;
            }
            string searchText = searchBox.Text;
            drmBrowserTree.Items.Clear();
            foreach (TreeListing item in lines)
            {
                if (item.DisplayText.Contains(searchText))
                    drmBrowserTree.Items.Add(item);
            }
        }

        private void replaceBtn_Click(object sender, RoutedEventArgs e)
        {

        }

        private void repackBtn_Click(object sender, RoutedEventArgs e)
        {
            if (imageListView.SelectedIndex == -1)
            {
                return;
            }
            System.Windows.Forms.OpenFileDialog inputFileDialog = new System.Windows.Forms.OpenFileDialog();
            inputFileDialog.Filter = "DDS File (*.dds)|*.dds";
            inputFileDialog.Multiselect = false;
            if (inputFileDialog.ShowDialog() != System.Windows.Forms.DialogResult.OK)
            {
                return;
            }
            string inputFileName = inputFileDialog.FileName;

            TreeListing tl = (TreeListing)drmBrowserTree.SelectedItem;
            ImageListing il = (ImageListing)imageListView.SelectedItem;
            Process TRExplorer = new Process();
            TRExplorer.StartInfo.FileName = "TRExplorer.exe";
            TRExplorer.StartInfo.Arguments = "\"" + tigerFileLocation + "\" pack " + tl.Id.ToString() + " " + il.Id + " \"" + inputFileName + "\"";
            TRExplorer.StartInfo.UseShellExecute = false;
            TRExplorer.StartInfo.RedirectStandardOutput = true;
            TRExplorer.Start();

            imageListView.Items.Clear();
            TRExplorer.WaitForExit();
        }

        private void extractBtn_Click(object sender, RoutedEventArgs e)
        {

        }
    }
}
