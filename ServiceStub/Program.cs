using System;
using System.ComponentModel;
using System.Configuration.Install;
using System.IO;
using System.Linq;
using System.ServiceProcess;

namespace ServiceStub
{
    class Program
    {
        public const string PublicName = "svcname";
        public const string PublicDescription = "Service description";

        static void Main(string[] args)
        {
            AppDomain.CurrentDomain.UnhandledException += CurrentDomain_UnhandledException;
            if (args.Length > 0) {
                switch (args[0]) {
                    case "-c":
                        Log = Console.WriteLine;
                        new Service().TestRun(args.Skip(1).ToArray());
                        Console.WriteLine("Press any key to exit.");
                        Console.ReadKey();
                        break;
                    case "-i":
                    case "-u":
                        var ins = new AssemblyInstaller(typeof(Program).Assembly.Location, new string[0]) {
                            UseNewContext = true
                        };
                        if (args[0] == "-i")
                            ins.Install(null);
                        else
                            ins.Uninstall(null);

                        ins.Commit(null);
                        break;
                    case "-s":
                        new ServiceController(PublicName).Start();
                        break;
                    default:
                        Console.Write(@"Unknown switch. Use one of these:
-c      Console: use for test run
-i      Install service
-u      Uninstall service
-s      Start service
");
                        break;
                }
            } else {
                Log = LogToFile;
                RotateLog();
                ServiceBase.Run(new Service());
            }
        }

        static void CurrentDomain_UnhandledException(object sender, UnhandledExceptionEventArgs e)
        {
            Log("UnhandledException: " + e.ExceptionObject);
        }

        #region logging stuff

        public delegate void LogDelegate(string format, params object[] args);

        public static LogDelegate Log = Console.WriteLine;

        private static DateTime lastRotated;
        private static StreamWriter FileLog;
        private const string logDir = @"logs";

        private static void RotateLog()
        {
            lastRotated = DateTime.Now;
            if (!Directory.Exists(logDir))
                Directory.CreateDirectory(logDir);

            FileLog = new StreamWriter(new FileStream(
                logDir + "\\" + lastRotated.ToString("yyyy-MM-dd HH.mm.ss") + ".log",
                FileMode.CreateNew, FileAccess.Write, FileShare.ReadWrite));
        }


        private static void LogToFile(string format, object[] args)
        {
            lock (FileLog) {
                FileLog.WriteLine(format, args);
                FileLog.Flush();
            }
        }

        #endregion

        #region Installer

        [RunInstaller(true)]
        public class Installer : System.Configuration.Install.Installer
        {
            public Installer()
            {
                Installers.Add(new ServiceInstaller {
                    StartType = ServiceStartMode.Automatic,
                    ServiceName = PublicName,
                    DisplayName = PublicName,
                    Description = PublicDescription,
                    ServicesDependedOn = new[] { "Tcpip" },
                });
                Installers.Add(new ServiceProcessInstaller {
                    Account = ServiceAccount.LocalSystem,
                });
            }
        }

        #endregion
    }
}
