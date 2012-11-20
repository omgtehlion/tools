using System;

namespace ServiceStub
{
    public class Service : System.ServiceProcess.ServiceBase
    {
        public Service()
        {
            ServiceName = Program.PublicName;
        }

        protected override void OnStart(string[] args)
        {
            // write service startup logic here
        }

        protected override void OnStop()
        {
        }

        internal void TestRun(string[] args)
        {
            OnStart(args);
        }
    }
}
