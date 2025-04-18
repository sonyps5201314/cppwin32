namespace CsWin32Test
{
    internal class Program
    {
        static unsafe void Main(string[] args)
        {
            int s = sizeof(Windows.Win32.System.Kernel.SLIST_ENTRY);
            Console.WriteLine("Hello, World!");
        }
    }
}
